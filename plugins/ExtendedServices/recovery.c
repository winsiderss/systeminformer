/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2015-2023
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
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
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
    _In_ HWND WindowHandle,
    _In_ PSERVICE_RECOVERY_CONTEXT Context
    )
{
    SC_ACTION_TYPE action1;
    SC_ACTION_TYPE action2;
    SC_ACTION_TYPE actionS;
    BOOLEAN enableRestart;
    BOOLEAN enableReboot;
    BOOLEAN enableCommand;

    action1 = ComboBoxToServiceAction(GetDlgItem(WindowHandle, IDC_FIRSTFAILURE));
    action2 = ComboBoxToServiceAction(GetDlgItem(WindowHandle, IDC_SECONDFAILURE));
    actionS = ComboBoxToServiceAction(GetDlgItem(WindowHandle, IDC_SUBSEQUENTFAILURES));

    EnableWindow(GetDlgItem(WindowHandle, IDC_ENABLEFORERRORSTOPS), Context->EnableFlagCheckBox);

    enableRestart = action1 == SC_ACTION_RESTART || action2 == SC_ACTION_RESTART || actionS == SC_ACTION_RESTART;
    enableReboot = action1 == SC_ACTION_REBOOT || action2 == SC_ACTION_REBOOT || actionS == SC_ACTION_REBOOT;
    enableCommand = action1 == SC_ACTION_RUN_COMMAND || action2 == SC_ACTION_RUN_COMMAND || actionS == SC_ACTION_RUN_COMMAND;

    EnableWindow(GetDlgItem(WindowHandle, IDC_RESTARTSERVICEAFTER_LABEL), enableRestart);
    EnableWindow(GetDlgItem(WindowHandle, IDC_RESTARTSERVICEAFTER), enableRestart);
    EnableWindow(GetDlgItem(WindowHandle, IDC_RESTARTSERVICEAFTER_MINUTES), enableRestart);

    EnableWindow(GetDlgItem(WindowHandle, IDC_RESTARTCOMPUTEROPTIONS), enableReboot);

    EnableWindow(GetDlgItem(WindowHandle, IDC_RUNPROGRAM_GROUP), enableCommand);
    EnableWindow(GetDlgItem(WindowHandle, IDC_RUNPROGRAM_LABEL), enableCommand);
    EnableWindow(GetDlgItem(WindowHandle, IDC_RUNPROGRAM), enableCommand);
    EnableWindow(GetDlgItem(WindowHandle, IDC_BROWSE), enableCommand);
    EnableWindow(GetDlgItem(WindowHandle, IDC_RUNPROGRAM_INFO), enableCommand);
}

NTSTATUS EspLoadRecoveryInfo(
    _In_ HWND WindowHandle,
    _In_ PSERVICE_RECOVERY_CONTEXT Context
    )
{
    NTSTATUS status;
    SC_HANDLE serviceHandle;
    LPSERVICE_FAILURE_ACTIONS failureActions;
    SERVICE_FAILURE_ACTIONS_FLAG failureActionsFlag;
    SC_ACTION_TYPE lastType;
    ULONG i;

    status = PhOpenService(&serviceHandle, SERVICE_QUERY_CONFIG, PhGetString(Context->ServiceItem->Name));

    if (!NT_SUCCESS(status))
        return status;

    status = PhQueryServiceVariableSize(serviceHandle, SERVICE_CONFIG_FAILURE_ACTIONS, &failureActions);

    if (!NT_SUCCESS(status))
    {
        PhCloseServiceHandle(serviceHandle);
        return status;
    }

    // Failure action types

    Context->NumberOfActions = failureActions->cActions;

    if (failureActions->cActions != 0 && failureActions->cActions != 3)
        status = STATUS_SOME_NOT_MAPPED;

    // If failure actions are not defined for a particular fail count, the
    // last failure action is used. Here we duplicate this behaviour when there
    // are fewer than 3 failure actions.
    lastType = SC_ACTION_NONE;

    ServiceActionToComboBox(GetDlgItem(WindowHandle, IDC_FIRSTFAILURE),
        failureActions->cActions >= 1 ? (lastType = failureActions->lpsaActions[0].Type) : lastType);
    ServiceActionToComboBox(GetDlgItem(WindowHandle, IDC_SECONDFAILURE),
        failureActions->cActions >= 2 ? (lastType = failureActions->lpsaActions[1].Type) : lastType);
    ServiceActionToComboBox(GetDlgItem(WindowHandle, IDC_SUBSEQUENTFAILURES),
        failureActions->cActions >= 3 ? (lastType = failureActions->lpsaActions[2].Type) : lastType);

    // Reset fail count after

    PhSetDialogItemValue(WindowHandle, IDC_RESETFAILCOUNT, failureActions->dwResetPeriod / (60 * 60 * 24), FALSE); // s to days

    // Restart service after

    PhSetDialogItemText(WindowHandle, IDC_RESTARTSERVICEAFTER, L"1");

    for (i = 0; i < failureActions->cActions; i++)
    {
        if (failureActions->lpsaActions[i].Type == SC_ACTION_RESTART)
        {
            if (failureActions->lpsaActions[i].Delay != 0)
            {
                PhSetDialogItemValue(WindowHandle, IDC_RESTARTSERVICEAFTER,
                    failureActions->lpsaActions[i].Delay / (1000 * 60), FALSE); // ms to min
            }

            break;
        }
    }

    // Enable actions for stops with errors

    if (NT_SUCCESS(PhQueryServiceConfig2(
        serviceHandle,
        SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
        &failureActionsFlag,
        sizeof(SERVICE_FAILURE_ACTIONS_FLAG),
        NULL
        )))
    {
        Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLEFORERRORSTOPS),
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

    PhSetDialogItemText(WindowHandle, IDC_RUNPROGRAM, failureActions->lpCommand);

    PhFree(failureActions);
    PhCloseServiceHandle(serviceHandle);

    return status;
}

INT_PTR CALLBACK EspServiceRecoveryDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_RECOVERY_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_RECOVERY_CONTEXT));
        memset(context, 0, sizeof(SERVICE_RECOVERY_CONTEXT));

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
            NTSTATUS status;
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;

            context->ServiceItem = serviceItem;

            EspAddServiceActionStrings(GetDlgItem(WindowHandle, IDC_FIRSTFAILURE));
            EspAddServiceActionStrings(GetDlgItem(WindowHandle, IDC_SECONDFAILURE));
            EspAddServiceActionStrings(GetDlgItem(WindowHandle, IDC_SUBSEQUENTFAILURES));

            status = EspLoadRecoveryInfo(WindowHandle, context);

            if (status == STATUS_SOME_NOT_MAPPED)
            {
                if (context->NumberOfActions > 3)
                {
                    PhShowWarning(
                        WindowHandle,
                        L"The service has %lu failure actions configured, but this program only supports editing 3. "
                        L"If you save the recovery information using this program, the additional failure actions will be lost.",
                        context->NumberOfActions
                        );
                }
            }
            else if (!NT_SUCCESS(status))
            {
                PPH_STRING errorMessage = PhGetNtMessage(status);

                PhSetDialogItemText(WindowHandle, IDC_RESETFAILCOUNT, L"0");

                context->EnableFlagCheckBox = TRUE;
                EnableWindow(GetDlgItem(WindowHandle, IDC_ENABLEFORERRORSTOPS), TRUE);

                PhShowWarning(
                    WindowHandle,
                    L"Unable to query service recovery information: %s",
                    PhGetStringOrDefault(errorMessage, L"Unknown error.")
                    );

                PhClearReference(&errorMessage);
            }

            EspFixControls(WindowHandle, context);

            context->Ready = TRUE;

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
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
                        EspFixControls(WindowHandle, context);
                    }
                }
                break;
            case IDC_RESTARTCOMPUTEROPTIONS:
                {
                    PhDialogBox(
                        PluginInstance->DllBase,
                        MAKEINTRESOURCE(IDD_RESTARTCOMP),
                        WindowHandle,
                        RestartComputerDlgProc,
                        context
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

                    fileName = PhaGetDlgItemText(WindowHandle, IDC_RUNPROGRAM);
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);

                    if (PhShowFileDialog(WindowHandle, fileDialog))
                    {
                        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
                        PhSetDialogItemText(WindowHandle, IDC_RUNPROGRAM, fileName->Buffer);
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
                    SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, FALSE);
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

                    SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, PSNRET_NOERROR);

                    if (!context->Dirty)
                    {
                        return TRUE;
                    }

                    // Build the failure actions structure.

                    failureActions.dwResetPeriod = PhGetDialogItemValue(WindowHandle, IDC_RESETFAILCOUNT) * 60 * 60 * 24;
                    failureActions.lpRebootMsg = PhGetStringOrEmpty(context->RebootMessage);
                    failureActions.lpCommand = PhaGetDlgItemText(WindowHandle, IDC_RUNPROGRAM)->Buffer;
                    failureActions.cActions = 3;
                    failureActions.lpsaActions = actions;

                    actions[0].Type = ComboBoxToServiceAction(GetDlgItem(WindowHandle, IDC_FIRSTFAILURE));
                    actions[1].Type = ComboBoxToServiceAction(GetDlgItem(WindowHandle, IDC_SECONDFAILURE));
                    actions[2].Type = ComboBoxToServiceAction(GetDlgItem(WindowHandle, IDC_SUBSEQUENTFAILURES));

                    restartServiceAfter = PhGetDialogItemValue(WindowHandle, IDC_RESTARTSERVICEAFTER) * 1000 * 60;

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

                    status = PhOpenService(
                        &serviceHandle,
                        SERVICE_CHANGE_CONFIG | (enableRestart ? SERVICE_START : 0), // SC_ACTION_RESTART requires SERVICE_START
                        PhGetString(serviceItem->Name)
                        );

                    if (NT_SUCCESS(status))
                    {
                        status = PhChangeServiceConfig2(
                            serviceHandle,
                            SERVICE_CONFIG_FAILURE_ACTIONS,
                            &failureActions
                            );

                        if (NT_SUCCESS(status))
                        {
                            if (context->EnableFlagCheckBox)
                            {
                                SERVICE_FAILURE_ACTIONS_FLAG failureActionsFlag;

                                failureActionsFlag.fFailureActionsOnNonCrashFailures =
                                    Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLEFORERRORSTOPS)) == BST_CHECKED;

                                PhChangeServiceConfig2(
                                    serviceHandle,
                                    SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
                                    &failureActionsFlag
                                    );
                            }

                            PhCloseServiceHandle(serviceHandle);
                        }
                        else
                        {
                            PhCloseServiceHandle(serviceHandle);
                            goto ErrorCase;
                        }
                    }
                    else
                    {
                        if (status == STATUS_ACCESS_DENIED && !PhGetOwnTokenAttributes().Elevated)
                        {
                            // Elevate using phsvc.
                            if (PhUiConnectToPhSvc(WindowHandle, FALSE))
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
                                            Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLEFORERRORSTOPS)) == BST_CHECKED;

                                        PhSvcCallChangeServiceConfig2(
                                            serviceItem->Name->Buffer,
                                            SERVICE_CONFIG_FAILURE_ACTIONS_FLAG,
                                            &failureActionsFlag
                                            );
                                    }
                                }

                                PhUiDisconnectFromPhSvc();

                                if (!NT_SUCCESS(status))
                                    goto ErrorCase;
                            }
                            else
                            {
                                // User cancelled elevation.
                                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, PSNRET_INVALID);
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
                        PPH_STRING errorMessage = PhGetNtMessage(status);

                        if (PhShowMessage(
                            WindowHandle,
                            MB_ICONERROR | MB_RETRYCANCEL,
                            L"Unable to change service recovery information: %s",
                            PhGetStringOrDefault(errorMessage, L"Unknown error.")
                            ) == IDRETRY)
                        {
                            SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, PSNRET_INVALID);
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
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return FALSE;
}

INT_PTR CALLBACK RestartComputerDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_RECOVERY_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PSERVICE_RECOVERY_CONTEXT)lParam;
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
            PhCenterWindow(WindowHandle, GetParent(WindowHandle));

            PhSetDialogItemValue(WindowHandle, IDC_RESTARTCOMPAFTER, context->RebootAfter / (1000 * 60), FALSE); // ms to min
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLERESTARTMESSAGE), context->RebootMessage ? BST_CHECKED : BST_UNCHECKED);
            PhSetDialogItemText(WindowHandle, IDC_RESTARTMESSAGE, PhGetString(context->RebootMessage));

            PhSetDialogFocus(WindowHandle, GetDlgItem(WindowHandle, IDC_RESTARTCOMPAFTER));
            Edit_SetSel(GetDlgItem(WindowHandle, IDC_RESTARTCOMPAFTER), 0, -1);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
                break;
            case IDOK:
                {
                    context->RebootAfter = PhGetDialogItemValue(WindowHandle, IDC_RESTARTCOMPAFTER) * 1000 * 60;

                    if (Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLERESTARTMESSAGE)) == BST_CHECKED)
                        PhMoveReference(&context->RebootMessage, PhGetWindowText(GetDlgItem(WindowHandle, IDC_RESTARTMESSAGE)));
                    else
                        PhClearReference(&context->RebootMessage);

                    context->Dirty = TRUE;

                    EndDialog(WindowHandle, IDOK);
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
                    PhSetDialogItemText(WindowHandle, IDC_RESTARTMESSAGE, message->Buffer);

                    if (allocated)
                        PhFree(computerName);

                    Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLERESTARTMESSAGE), BST_CHECKED);
                }
                break;
            case IDC_RESTARTMESSAGE:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        // A zero length restart message disables it, so we might as well uncheck the box.
                        Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLERESTARTMESSAGE),
                            PhGetWindowTextLength(GetDlgItem(WindowHandle, IDC_RESTARTMESSAGE)) != 0 ? BST_CHECKED : BST_UNCHECKED);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

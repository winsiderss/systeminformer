/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2024
 *
 */

/*
 * These are a set of consistent functions which will perform actions on objects such as processes,
 * threads and services, while displaying any necessary prompts and error messages. Automatic
 * elevation can also easily be added if necessary.
 */

#include <phapp.h>
#include <actions.h>

#include <winsta.h>

#include <kphuser.h>
#include <ksisup.h>
#include <mapldr.h>
#include <secwmi.h>
#include <settings.h>
#include <svcsup.h>

#include <apiimport.h>
#include <bcd.h>
#include <emenu.h>
#include <hndlprv.h>
#include <memprv.h>
#include <modprv.h>
#include <netprv.h>
#include <phsvccl.h>
#include <procprv.h>
#include <srvprv.h>
#include <thrdprv.h>

static PH_STRINGREF DangerousProcesses[] =
{
    PH_STRINGREF_INIT(L"csrss.exe"), PH_STRINGREF_INIT(L"dwm.exe"), PH_STRINGREF_INIT(L"logonui.exe"),
    PH_STRINGREF_INIT(L"lsass.exe"), PH_STRINGREF_INIT(L"lsm.exe"), PH_STRINGREF_INIT(L"services.exe"),
    PH_STRINGREF_INIT(L"smss.exe"), PH_STRINGREF_INIT(L"wininit.exe"), PH_STRINGREF_INIT(L"winlogon.exe")
};

static volatile LONG PhSvcReferenceCount = 0;
static PH_PHSVC_MODE PhSvcCurrentMode;
static PH_QUEUED_LOCK PhSvcStartLock = PH_QUEUED_LOCK_INIT;

HRESULT CALLBACK PhpElevateActionCallbackProc(
    _In_ HWND WindowHandle,
    _In_ UINT Notification,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR Context
    )
{
    switch (Notification)
    {
    case TDN_CREATED:
        SendMessage(WindowHandle, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDYES, TRUE);
        break;
    }

    return S_OK;
}

_Success_(return)
BOOLEAN PhpShowElevatePrompt(
    _In_ HWND WindowHandle,
    _In_ PWSTR Message,
    _In_ PVOID Context,
    _Out_ PINT32 Button
    )
{
    TASKDIALOGCONFIG config;
    TASKDIALOG_BUTTON buttons[1] =
    {
        { IDYES, L"Continue"}
    };
    INT button;

    // Currently the error dialog box is similar to the one displayed
    // when you try to label a drive in Windows Explorer. It's much better
    // than the clunky dialog in PH 1.x.

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.hwndParent = WindowHandle;
    config.hInstance = PhInstanceHandle;
    config.dwFlags = IsWindowVisible(WindowHandle) ? TDF_POSITION_RELATIVE_TO_WINDOW : 0;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = TD_ERROR_ICON;
    config.pszMainInstruction = PhaConcatStrings2(Message, L".")->Buffer;
    config.pszContent = L"You will need to provide administrator permission. "
        L"Click Continue to complete this operation.";
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;

    config.cButtons = 1;
    config.pButtons = buttons;
    config.nDefaultButton = IDYES;

    config.pfCallback = PhpElevateActionCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    if (HR_SUCCESS(TaskDialogIndirect(
        &config,
        &button,
        NULL,
        NULL
        )))
    {
        *Button = button;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * Shows an error, prompts for elevation, and executes a command.
 *
 * \param WindowHandle The window to display user interface components on.
 * \param Connected A variable which receives TRUE if the elevated
 * action succeeded or FALSE if the action failed.
 *
 * \return TRUE if the user was prompted for elevation, otherwise
 * FALSE, in which case you need to show your own error message.
 */
_Success_(return)
BOOLEAN PhpElevationLevelAndConnectToPhSvc(
    _In_ HWND WindowHandle,
    _Out_opt_ PBOOLEAN Connected
    )
{
    PH_ACTION_ELEVATION_LEVEL elevationLevel;

    *Connected = FALSE;

    if (PhGetOwnTokenAttributes().Elevated)
        return FALSE;

    elevationLevel = PhGetIntegerSetting(L"ElevationLevel");

    if (elevationLevel == NeverElevateAction)
        return FALSE;

    // Try to connect now so we can avoid prompting the user.
    if (PhUiConnectToPhSvc(WindowHandle, TRUE))
    {
        *Connected = TRUE;
        return TRUE;
    }

    if (
        elevationLevel == PromptElevateAction ||
        elevationLevel == AlwaysElevateAction
        )
    {
        *Connected = PhUiConnectToPhSvc(WindowHandle, FALSE);
        return TRUE;
    }

    return FALSE;
}

/**
 * Shows an error, prompts for elevation, and connects to phsvc.
 *
 * \param WindowHandle The window to display user interface components on.
 * \param Message A message describing the operation that failed.
 * \param Status A NTSTATUS value.
 * \param Connected A variable which receives TRUE if the user
 * elevated the action and phsvc was started, or FALSE if the user
 * cancelled elevation. If the value is TRUE, you need to
 * perform any necessary phsvc calls and use PhUiDisconnectFromPhSvc()
 * to disconnect from phsvc.
 *
 * \return TRUE if the user was prompted for elevation, otherwise
 * FALSE, in which case you need to show your own error message.
 */
BOOLEAN PhpShowErrorAndConnectToPhSvc(
    _In_ HWND WindowHandle,
    _In_ PWSTR Message,
    _In_ NTSTATUS Status,
    _Out_ PBOOLEAN Connected
    )
{
    PH_ACTION_ELEVATION_LEVEL elevationLevel;
    INT button = IDNO;

    *Connected = FALSE;

    if (!(Status == STATUS_ACCESS_DENIED || Status == STATUS_PRIVILEGE_NOT_HELD))
        return FALSE;

    if (PhGetOwnTokenAttributes().Elevated)
        return FALSE;

    elevationLevel = PhGetIntegerSetting(L"ElevationLevel");

    if (elevationLevel == NeverElevateAction)
        return FALSE;

    // Try to connect now so we can avoid prompting the user.
    if (PhUiConnectToPhSvc(WindowHandle, TRUE))
    {
        *Connected = TRUE;
        return TRUE;
    }

    if (elevationLevel == PromptElevateAction)
    {
        if (!PhpShowElevatePrompt(WindowHandle, Message, NULL, &button))
            return FALSE;
    }

    if (elevationLevel == AlwaysElevateAction || button == IDYES)
    {
        *Connected = PhUiConnectToPhSvc(WindowHandle, FALSE);
        return TRUE;
    }

    return FALSE;
}

/**
 * Connects to phsvc.
 *
 * \param WindowHandle The window to display user interface components on.
 * \param ConnectOnly TRUE to only try to connect to phsvc, otherwise
 * FALSE to try to elevate and start phsvc if the initial connection
 * attempt failed.
 */
BOOLEAN PhUiConnectToPhSvc(
    _In_opt_ HWND WindowHandle,
    _In_ BOOLEAN ConnectOnly
    )
{
    return PhUiConnectToPhSvcEx(WindowHandle, ElevatedPhSvcMode, ConnectOnly);
}

VOID PhpGetPhSvcPortName(
    _In_ PH_PHSVC_MODE Mode,
    _Out_ PUNICODE_STRING PortName
    )
{
    switch (Mode)
    {
    case ElevatedPhSvcMode:
        if (!PhIsExecutingInWow64())
            RtlInitUnicodeString(PortName, PHSVC_PORT_NAME);
        else
            RtlInitUnicodeString(PortName, PHSVC_WOW64_PORT_NAME);
        break;
    case Wow64PhSvcMode:
        RtlInitUnicodeString(PortName, PHSVC_WOW64_PORT_NAME);
        break;
    default:
        PhRaiseStatus(STATUS_INVALID_PARAMETER);
        break;
    }
}

BOOLEAN PhpStartPhSvcProcess(
    _In_opt_ HWND WindowHandle,
    _In_ PH_PHSVC_MODE Mode
    )
{
    switch (Mode)
    {
    case ElevatedPhSvcMode:
        if (NT_SUCCESS(PhShellProcessHacker(
            WindowHandle,
            L"-phsvc",
            SW_HIDE,
            PH_SHELL_EXECUTE_ADMIN,
            0,
            0,
            NULL
            )))
        {
            return TRUE;
        }

        break;
    case Wow64PhSvcMode:
        {
            static PH_STRINGREF relativeFileNames[] =
            {
                PH_STRINGREF_INIT(L"\\x86\\"),
#ifdef DEBUG
                PH_STRINGREF_INIT(L"\\..\\Debug32\\"),
                PH_STRINGREF_INIT(L"\\..\\Release32\\")
#endif
            };
            ULONG i;
            PPH_STRING applicationDirectory;
            PPH_STRING applicationFileName;

            if (!(applicationDirectory = PhGetApplicationDirectoryWin32()))
                return FALSE;
            if (!(applicationFileName = PhGetApplicationFileNameWin32()))
                return FALSE;

            PhMoveReference(&applicationFileName, PhGetBaseName(applicationFileName));

            for (i = 0; i < RTL_NUMBER_OF(relativeFileNames); i++)
            {
                PPH_STRING fileName;
                PPH_STRING fileFullPath;

                fileName = PhConcatStringRef3(
                    &applicationDirectory->sr,
                    &relativeFileNames[i],
                    &applicationFileName->sr
                    );

                if (NT_SUCCESS(PhGetFullPath(PhGetString(fileName), &fileFullPath, NULL)))
                {
                    PhMoveReference(&fileName, fileFullPath);
                }

                if (PhDoesFileExistWin32(PhGetString(fileName)))
                {
                    if (NT_SUCCESS(PhShellProcessHackerEx(
                        WindowHandle,
                        PhGetString(fileName),
                        L"-phsvc",
                        SW_HIDE,
                        PH_SHELL_EXECUTE_DEFAULT,
                        0,
                        0,
                        NULL
                        )))
                    {
                        PhDereferenceObject(fileName);
                        PhDereferenceObject(applicationFileName);
                        PhDereferenceObject(applicationDirectory);
                        return TRUE;
                    }
                }

                PhDereferenceObject(fileName);
            }

            PhDereferenceObject(applicationFileName);
            PhDereferenceObject(applicationDirectory);
        }
        break;
    }

    return FALSE;
}

/**
 * Connects to phsvc.
 *
 * \param WindowHandle The window to display user interface components on.
 * \param Mode The type of phsvc instance to connect to.
 * \param ConnectOnly TRUE to only try to connect to phsvc, otherwise
 * FALSE to try to elevate and start phsvc if the initial connection
 * attempt failed.
 */
BOOLEAN PhUiConnectToPhSvcEx(
    _In_opt_ HWND WindowHandle,
    _In_ PH_PHSVC_MODE Mode,
    _In_ BOOLEAN ConnectOnly
    )
{
    NTSTATUS status;
    BOOLEAN started;
    UNICODE_STRING portName;

    if (_InterlockedIncrementNoZero(&PhSvcReferenceCount))
    {
        if (PhSvcCurrentMode == Mode)
        {
            started = TRUE;
        }
        else
        {
            _InterlockedDecrement(&PhSvcReferenceCount);
            started = FALSE;
        }
    }
    else
    {
        PhAcquireQueuedLockExclusive(&PhSvcStartLock);

        if (_InterlockedExchange(&PhSvcReferenceCount, 0) == 0)
        {
            started = FALSE;
            PhpGetPhSvcPortName(Mode, &portName);

            // Try to connect first, then start the server if we failed.
            status = PhSvcConnectToServer(&portName, 0);

            if (NT_SUCCESS(status))
            {
                started = TRUE;
                PhSvcCurrentMode = Mode;
                _InterlockedIncrement(&PhSvcReferenceCount);
            }
            else if (!ConnectOnly)
            {
                // Prompt for elevation, and then try to connect to the server.

                if (PhpStartPhSvcProcess(WindowHandle, Mode))
                    started = TRUE;

                if (started)
                {
                    ULONG attempts = 10;

                    // Try to connect several times because the server may take
                    // a while to initialize.
                    do
                    {
                        status = PhSvcConnectToServer(&portName, 0);

                        if (NT_SUCCESS(status))
                            break;

                        PhDelayExecution(1000);

                    } while (--attempts != 0);

                    // Increment the reference count even if we failed.
                    // We don't want to prompt the user again.

                    PhSvcCurrentMode = Mode;
                    _InterlockedIncrement(&PhSvcReferenceCount);
                }
            }
        }
        else
        {
            if (PhSvcCurrentMode == Mode)
            {
                started = TRUE;
                _InterlockedIncrement(&PhSvcReferenceCount);
            }
            else
            {
                started = FALSE;
            }
        }

        PhReleaseQueuedLockExclusive(&PhSvcStartLock);
    }

    return started;
}

/**
 * Disconnects from phsvc.
 */
VOID PhUiDisconnectFromPhSvc(
    VOID
    )
{
    PhAcquireQueuedLockExclusive(&PhSvcStartLock);

    if (_InterlockedDecrement(&PhSvcReferenceCount) == 0)
    {
        PhSvcDisconnectFromServer();
    }

    PhReleaseQueuedLockExclusive(&PhSvcStartLock);
}

BOOLEAN PhUiLockComputer(
    _In_ HWND WindowHandle
    )
{
    if (LockWorkStation())
        return TRUE;
    else
        PhShowStatus(WindowHandle, L"Unable to lock the computer.", 0, PhGetLastError());

    return FALSE;
}

BOOLEAN PhUiLogoffComputer(
    _In_ HWND WindowHandle
    )
{
    if (ExitWindowsEx(EWX_LOGOFF, 0))
        return TRUE;
    else
        PhShowStatus(WindowHandle, L"Unable to log off the computer.", 0, GetLastError());

    return FALSE;
}

BOOLEAN PhUiSleepComputer(
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;

    if (NT_SUCCESS(status = NtInitiatePowerAction(
        PowerActionSleep,
        PowerSystemSleeping1,
        0,
        FALSE
        )))
        return TRUE;
    else
        PhShowStatus(WindowHandle, L"Unable to sleep the computer.", status, 0);

    return FALSE;
}

BOOLEAN PhUiHibernateComputer(
    _In_ HWND WindowHandle
    )
{
    NTSTATUS status;

    if (NT_SUCCESS(status = NtInitiatePowerAction(
        PowerActionHibernate,
        PowerSystemSleeping1,
        0,
        FALSE
        )))
        return TRUE;
    else
        PhShowStatus(WindowHandle, L"Unable to hibernate the computer.", status, 0);

    return FALSE;
}

BOOLEAN PhUiRestartComputer(
    _In_ HWND WindowHandle,
    _In_ PH_POWERACTION_TYPE Action,
    _In_ ULONG Flags
    )
{
    switch (Action)
    {
    case PH_POWERACTION_TYPE_WIN32:
        {
            if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                WindowHandle,
                L"restart",
                L"the computer",
                NULL,
                FALSE
                ))
            {
                ULONG status = PhInitiateShutdown(PH_SHUTDOWN_RESTART | Flags);

                if (status == ERROR_SUCCESS)
                    return TRUE;

                PhShowStatus(WindowHandle, L"Unable to restart the computer.", 0, status);

                //if (ExitWindowsEx(EWX_REBOOT | EWX_BOOTOPTIONS, 0))
                //    return TRUE;
                //else
                //    PhShowStatus(WindowHandle, L"Unable to restart the computer.", 0, GetLastError());
            }
        }
        break;
    case PH_POWERACTION_TYPE_NATIVE:
        {
            PPH_STRING messageText;

            messageText = PhaFormatString(
                L"This option %s %s in an disorderly manner and may cause corrupted files or instability in the system.",
                L"preforms a hard",
                L"restart");

            // Ignore the EnableWarnings preference and always show the warning prompt. (dmex)
            if (PhShowConfirmMessage(
                WindowHandle,
                L"restart",
                L"the computer",
                messageText->Buffer,
                TRUE
                ))
            {
                NTSTATUS status;

                status = NtShutdownSystem(ShutdownReboot);

                if (NT_SUCCESS(status))
                    return TRUE;

                PhShowStatus(WindowHandle, L"Unable to restart the computer.", status, 0);
            }
        }
        break;
    case PH_POWERACTION_TYPE_CRITICAL:
        {
            PPH_STRING messageText;

            messageText = PhaFormatString(
                L"This option %s %s in an disorderly manner and may cause corrupted files or instability in the system.",
                L"forces a critical",
                L"restart");

            // Ignore the EnableWarnings preference and always show the warning prompt. (dmex)
            if (PhShowConfirmMessage(
                WindowHandle,
                L"restart",
                L"the computer",
                messageText->Buffer,
                TRUE
                ))
            {
                NTSTATUS status;

                status = NtSetSystemPowerState(
                    PowerActionShutdownReset,
                    PowerSystemShutdown,
                    POWER_ACTION_CRITICAL
                    );
                //status = NtInitiatePowerAction(
                //    PowerActionShutdownReset,
                //    PowerSystemShutdown,
                //    POWER_ACTION_CRITICAL,
                //    FALSE
                //    );

                if (NT_SUCCESS(status))
                    return TRUE;

                PhShowStatus(WindowHandle, L"Unable to restart the computer.", status, 0);
            }
        }
        break;
    case PH_POWERACTION_TYPE_ADVANCEDBOOT:
        {
            if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                WindowHandle,
                L"restart",
                L"the computer",
                NULL,
                FALSE
                ))
            {
                NTSTATUS status;

                status = PhBcdSetAdvancedOptionsOneTime(
                    TRUE
                    );

                if (NT_SUCCESS(status))
                {
                    status = PhInitiateShutdown(PH_SHUTDOWN_RESTART);

                    if (status == ERROR_SUCCESS)
                        return TRUE;

                    PhShowStatus(WindowHandle, L"Unable to configure the advanced boot options.", 0, status);
                }
                else
                {
                    PhShowStatus(WindowHandle, L"Unable to configure the advanced boot options.", status, 0);
                }
            }
        }
        break;
    case PH_POWERACTION_TYPE_FIRMWAREBOOT:
        {
            if (!PhGetOwnTokenAttributes().Elevated)
            {
                PhShowMessage2(
                    WindowHandle,
                    TD_OK_BUTTON,
                    TD_ERROR_ICON,
                    L"Unable to restart to firmware options.",
                    L"Make sure System Informer is running with administrative privileges."
                    );
                break;
            }

            if (!NT_SUCCESS(PhAdjustPrivilege(NULL, SE_SYSTEM_ENVIRONMENT_PRIVILEGE, TRUE)))
            {
                PhShowMessage2(
                    WindowHandle,
                    TD_OK_BUTTON,
                    TD_ERROR_ICON,
                    L"Unable to restart to firmware options.",
                    L"Make sure System Informer is running with administrative privileges."
                    );
                break;
            }

            if (!PhIsFirmwareSupported())
            {
                PhShowMessage2(
                    WindowHandle,
                    TD_OK_BUTTON,
                    TD_ERROR_ICON,
                    L"Unable to restart to firmware options.",
                    L"This machine does not have UEFI support."
                    );
                break;
            }

            if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                WindowHandle,
                L"restart",
                L"the computer",
                NULL,
                FALSE
                ))
            {
                NTSTATUS status;

                status = PhSetSystemEnvironmentBootToFirmware();

                if (NT_SUCCESS(status))
                {
                    status = PhInitiateShutdown(PH_SHUTDOWN_RESTART);

                    if (status == ERROR_SUCCESS)
                        return TRUE;

                    PhShowStatus(WindowHandle, L"Unable to restart the computer.", 0, status);
                }
                else
                {
                    PhShowStatus(WindowHandle, L"Unable to restart the computer.", status, 0);
                }
            }
        }
        break;
    case PH_POWERACTION_TYPE_UPDATE:
        {
            if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                WindowHandle,
                L"update and restart",
                L"the computer",
                NULL,
                FALSE
                ))
            {
                ULONG status = PhInitiateShutdown(PH_SHUTDOWN_RESTART | PH_SHUTDOWN_INSTALL_UPDATES);

                if (status == ERROR_SUCCESS)
                    return TRUE;

                PhShowStatus(WindowHandle, L"Unable to restart the computer.", 0, status);
            }
        }
        break;
    case PH_POWERACTION_TYPE_WDOSCAN:
        {
            if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                WindowHandle,
                L"restart",
                L"the computer for Windows Defender Offline Scan",
                NULL,
                FALSE
                ))
            {
                HRESULT status = PhRestartDefenderOfflineScan();

                if (status == S_OK)
                    return TRUE;

                if ((status & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0))
                {
                    PhShowStatus(WindowHandle, L"Unable to restart the computer.", 0, HRESULT_CODE(status));
                }
                else
                {
                    PhShowStatus(WindowHandle, L"Unable to restart the computer.", STATUS_UNSUCCESSFUL, 0);
                }
            }
        }
        break;
    }

    return FALSE;
}

BOOLEAN PhUiShutdownComputer(
    _In_ HWND WindowHandle,
    _In_ PH_POWERACTION_TYPE Action,
    _In_ ULONG Flags
    )
{
    switch (Action)
    {
    case PH_POWERACTION_TYPE_WIN32:
        {
            if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                WindowHandle,
                L"shut down",
                L"the computer",
                NULL,
                FALSE
                ))
            {
                ULONG status = PhInitiateShutdown(PH_SHUTDOWN_POWEROFF | Flags);

                if (status == ERROR_SUCCESS)
                    return TRUE;

                PhShowStatus(WindowHandle, L"Unable to shut down the computer.", 0, status);

                //if (ExitWindowsEx(EWX_POWEROFF | EWX_HYBRID_SHUTDOWN, 0))
                //    return TRUE;
                //else if (ExitWindowsEx(EWX_SHUTDOWN | EWX_HYBRID_SHUTDOWN, 0))
                //    return TRUE;
                //else
                //    PhShowStatus(WindowHandle, L"Unable to shut down the computer.", 0, GetLastError());
            }
        }
        break;
    case PH_POWERACTION_TYPE_NATIVE:
        {
            PPH_STRING messageText;

            messageText = PhaFormatString(
                L"This option %s %s in an disorderly manner and may cause corrupted files or instability in the system.",
                L"preforms a hard",
                L"shut down");

            // Ignore the EnableWarnings preference and always show the warning prompt. (dmex)
            if (PhShowConfirmMessage(
                WindowHandle,
                L"shut down",
                L"the computer",
                messageText->Buffer,
                TRUE
                ))
            {
                NTSTATUS status;

                status = NtShutdownSystem(ShutdownPowerOff);

                if (NT_SUCCESS(status))
                    return TRUE;

                PhShowStatus(WindowHandle, L"Unable to shut down the computer.", status, 0);
            }
        }
        break;
    case PH_POWERACTION_TYPE_CRITICAL:
        {
            PPH_STRING messageText;

            messageText = PhaFormatString(
                L"This option %s %s in an disorderly manner and may cause corrupted files or instability in the system.",
                L"forces a critical",
                L"shut down");

            // Ignore the EnableWarnings preference and always show the warning prompt. (dmex)
            if (PhShowConfirmMessage(
                WindowHandle,
                L"shut down",
                L"the computer",
                messageText->Buffer,
                TRUE
                ))
            {
                NTSTATUS status;

                status = NtSetSystemPowerState(
                    PowerActionShutdownOff,
                    PowerSystemShutdown,
                    POWER_ACTION_CRITICAL
                    );
                //status = NtInitiatePowerAction(
                //    PowerActionShutdownReset,
                //    PowerSystemShutdown,
                //    POWER_ACTION_CRITICAL,
                //    FALSE
                //    );

                if (NT_SUCCESS(status))
                    return TRUE;

                PhShowStatus(WindowHandle, L"Unable to shut down the computer.", status, 0);
            }
        }
        break;
    case PH_POWERACTION_TYPE_UPDATE:
        {
            if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                WindowHandle,
                L"update and shutdown",
                L"the computer",
                NULL,
                FALSE
                ))
            {
                ULONG status = PhInitiateShutdown(PH_SHUTDOWN_POWEROFF | PH_SHUTDOWN_INSTALL_UPDATES);

                if (status == ERROR_SUCCESS)
                    return TRUE;

                PhShowStatus(WindowHandle, L"Unable to shut down the computer.", 0, status);
            }
        }
        break;
    }

    return FALSE;
}

PVOID PhUiCreateComputerBootDeviceMenu(
    _In_ BOOLEAN DelayLoadMenu
    )
{
    PPH_EMENU_ITEM menuItem;
    PPH_LIST bootApplicationList;

    menuItem = PhCreateEMenuItem(PH_EMENU_DISABLED, ID_COMPUTER_RESTARTBOOTDEVICE, L"Restart to boot application", NULL, NULL);

    if (!PhGetOwnTokenAttributes().Elevated)
        return menuItem;

    if (!DelayLoadMenu)
    {
        BOOLEAN bootEnumerateAllObjects = !!PhGetIntegerSetting(L"EnableBootObjectsEnumerate");

        if (bootApplicationList = PhBcdQueryBootApplicationList(bootEnumerateAllObjects))
        {
            for (ULONG i = 0; i < bootApplicationList->Count; i++)
            {
                PPH_BCD_OBJECT_LIST entry = bootApplicationList->Items[i];
                PPH_EMENU_ITEM menuItemNew;

                menuItemNew = PhCreateEMenuItem(
                    PH_EMENU_TEXT_OWNED,
                    ID_COMPUTER_RESTARTBOOTDEVICE,
                    PhAllocateCopy(entry->ObjectName->Buffer, entry->ObjectName->Length + sizeof(UNICODE_NULL)),
                    NULL,
                    UlongToPtr(i)
                    );

                PhInsertEMenuItem(menuItem, menuItemNew, ULONG_MAX);
            }

            if (bootApplicationList->Count)
                PhSetEnabledEMenuItem(menuItem, TRUE);

            PhBcdDestroyBootApplicationList(bootApplicationList);
        }
    }

    return menuItem;
}

PVOID PhUiCreateComputerFirmwareDeviceMenu(
    _In_ BOOLEAN DelayLoadMenu
    )
{
    PPH_EMENU_ITEM menuItem;
    PPH_LIST firmwareApplicationList;

    menuItem = PhCreateEMenuItem(PH_EMENU_DISABLED, ID_COMPUTER_RESTARTFWDEVICE, L"Restart to firmware application", NULL, NULL);

    if (!PhGetOwnTokenAttributes().Elevated)
        return menuItem;

    if (!DelayLoadMenu)
    {
        if (firmwareApplicationList = PhBcdQueryFirmwareBootApplicationList())
        {
            for (ULONG i = 0; i < firmwareApplicationList->Count; i++)
            {
                PPH_BCD_OBJECT_LIST entry = firmwareApplicationList->Items[i];
                PPH_EMENU_ITEM menuItemNew;

                menuItemNew = PhCreateEMenuItem(
                    PH_EMENU_TEXT_OWNED,
                    ID_COMPUTER_RESTARTFWDEVICE,
                    PhAllocateCopy(entry->ObjectName->Buffer, entry->ObjectName->Length + sizeof(UNICODE_NULL)),
                    NULL,
                    UlongToPtr(i)
                    );

                PhInsertEMenuItem(menuItem, menuItemNew, ULONG_MAX);
            }

            if (firmwareApplicationList->Count)
                PhSetEnabledEMenuItem(menuItem, TRUE);

            PhBcdDestroyBootApplicationList(firmwareApplicationList);
        }
    }

    return menuItem;
}

VOID PhUiHandleComputerBootApplicationMenu(
    _In_ HWND WindowHandle,
    _In_ ULONG MenuIndex
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    BOOLEAN bootEnumerateAllObjects;
    BOOLEAN bootUpdateFwBootObjects;
    PPH_LIST bootApplicationList;

    if (PhGetIntegerSetting(L"EnableWarnings") && !PhShowConfirmMessage(
        WindowHandle,
        L"restart",
        L"the computer",
        NULL,
        FALSE
        ))
    {
        return;
    }

    bootEnumerateAllObjects = !!PhGetIntegerSetting(L"EnableBootObjectsEnumerate");
    bootUpdateFwBootObjects = !!PhGetIntegerSetting(L"EnableUpdateDefaultFirmwareBootEntry");

    if (bootApplicationList = PhBcdQueryBootApplicationList(bootEnumerateAllObjects))
    {
        if (MenuIndex < bootApplicationList->Count)
        {
            PPH_BCD_OBJECT_LIST entry = bootApplicationList->Items[MenuIndex];

            status = PhBcdSetBootApplicationOneTime(entry->ObjectGuid, bootUpdateFwBootObjects);
        }

        PhBcdDestroyBootApplicationList(bootApplicationList);
    }

    if (NT_SUCCESS(status))
    {
        status = PhInitiateShutdown(PH_SHUTDOWN_RESTART);

        if (status != ERROR_SUCCESS)
        {
            PhShowStatus(WindowHandle, L"Unable to configure the boot application.", 0, status);
        }
    }
    else
    {
        PhShowStatus(WindowHandle, L"Unable to configure the boot application.", status, 0);
    }
}

VOID PhUiHandleComputerFirmwareApplicationMenu(
    _In_ HWND WindowHandle,
    _In_ ULONG MenuIndex
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPH_LIST firmwareApplicationList;

    if (PhGetIntegerSetting(L"EnableWarnings") && !PhShowConfirmMessage(
        WindowHandle,
        L"restart",
        L"the computer",
        NULL,
        FALSE
        ))
    {
        return;
    }

    if (firmwareApplicationList = PhBcdQueryFirmwareBootApplicationList())
    {
        if (MenuIndex < firmwareApplicationList->Count)
        {
            PPH_BCD_OBJECT_LIST entry = firmwareApplicationList->Items[MenuIndex];

            status = PhBcdSetFirmwareBootApplicationOneTime(entry->ObjectGuid);
        }

        PhBcdDestroyBootApplicationList(firmwareApplicationList);
    }

    if (NT_SUCCESS(status))
    {
        status = PhInitiateShutdown(PH_SHUTDOWN_RESTART);

        if (status != ERROR_SUCCESS)
        {
            PhShowStatus(WindowHandle, L"Unable to configure the boot application.", 0, status);
        }
    }
    else
    {
        PhShowStatus(WindowHandle, L"Unable to configure the boot application.", status, 0);
    }
}

typedef struct _PHP_USERSMENU_ENTRY
{
    ULONG SessionId;
    PPH_STRING UserName;
} PHP_USERSMENU_ENTRY, *PPHP_USERSMENU_ENTRY;

static int __cdecl PhpUsersMainMenuNameCompare(
    _In_ const void* Context,
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPHP_USERSMENU_ENTRY item1 = *(PPHP_USERSMENU_ENTRY*)elem1;
    PPHP_USERSMENU_ENTRY item2 = *(PPHP_USERSMENU_ENTRY*)elem2;

    return PhCompareString(item1->UserName, item2->UserName, TRUE);
}

VOID PhUiCreateSessionMenu(
    _In_ PVOID UsersMenuItem
    )
{
    PPH_LIST userSessionList;
    PSESSIONIDW sessions;
    ULONG numberOfSessions;
    ULONG i;

    userSessionList = PhCreateList(1);

    if (WinStationEnumerateW(WINSTATION_CURRENT_SERVER, &sessions, &numberOfSessions))
    {
        for (i = 0; i < numberOfSessions; i++)
        {
            WINSTATIONINFORMATION winStationInfo;
            ULONG returnLength;
            SIZE_T formatLength;
            PH_FORMAT format[5];
            PH_STRINGREF menuTextSr;
            WCHAR formatBuffer[0x100];

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

            if (winStationInfo.Domain[0] == UNICODE_NULL || winStationInfo.UserName[0] == UNICODE_NULL)
            {
                // Probably the Services or RDP-Tcp session.
                continue;
            }

            PhInitFormatU(&format[0], sessions[i].SessionId);
            PhInitFormatS(&format[1], L": ");
            PhInitFormatS(&format[2], winStationInfo.Domain);
            PhInitFormatC(&format[3], OBJ_NAME_PATH_SEPARATOR);
            PhInitFormatS(&format[4], winStationInfo.UserName);

            if (!PhFormatToBuffer(
                format,
                RTL_NUMBER_OF(format),
                formatBuffer,
                sizeof(formatBuffer),
                &formatLength
                ))
            {
                continue;
            }

            menuTextSr.Length = formatLength - sizeof(UNICODE_NULL);
            menuTextSr.Buffer = formatBuffer;

            {
                PPHP_USERSMENU_ENTRY entry;

                entry = PhCreateAlloc(sizeof(PHP_USERSMENU_ENTRY));
                entry->SessionId = sessions[i].SessionId;
                entry->UserName = PhCreateString2(&menuTextSr);

                PhAddItemList(userSessionList, entry);
            }
        }

        WinStationFreeMemory(sessions);
    }

    // Sort the users. (dmex)
    qsort_s(userSessionList->Items, userSessionList->Count, sizeof(PVOID), PhpUsersMainMenuNameCompare, NULL);

    // Update the users menu. (dmex)
    for (i = 0; i < userSessionList->Count; i++)
    {
        PPHP_USERSMENU_ENTRY entry;
        PPH_STRING escapedMenuText;
        PPH_EMENU_ITEM userMenu;

        entry = userSessionList->Items[i];
        escapedMenuText = PhEscapeStringForMenuPrefix(&entry->UserName->sr);
        userMenu = PhCreateEMenuItem(
            PH_EMENU_TEXT_OWNED,
            0,
            PhAllocateCopy(escapedMenuText->Buffer, escapedMenuText->Length + sizeof(UNICODE_NULL)),
            NULL,
            UlongToPtr(entry->SessionId)
            );
        PhDereferenceObject(escapedMenuText);
        PhDereferenceObject(entry->UserName);

        PhInsertEMenuItem(userMenu, PhCreateEMenuItem(0, ID_USER_CONNECT, L"&Connect", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(userMenu, PhCreateEMenuItem(0, ID_USER_DISCONNECT, L"&Disconnect", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(userMenu, PhCreateEMenuItem(0, ID_USER_LOGOFF, L"&Logoff", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(userMenu, PhCreateEMenuItem(0, ID_USER_REMOTECONTROL, L"Rem&ote control", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(userMenu, PhCreateEMenuItem(0, ID_USER_SENDMESSAGE, L"Send &message...", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(userMenu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(userMenu, PhCreateEMenuItem(0, ID_USER_PROPERTIES, L"P&roperties", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(UsersMenuItem, userMenu, ULONG_MAX);
    }

    PhDereferenceObjects(userSessionList->Items, userSessionList->Count);
    PhDereferenceObject(userSessionList);
}

BOOLEAN PhUiConnectSession(
    _In_ HWND WindowHandle,
    _In_ ULONG SessionId
    )
{
    BOOLEAN success = FALSE;
    PPH_STRING selectedChoice = NULL;
    PPH_STRING oldSelectedChoice = NULL;

    // Try once with no password.
    if (WinStationConnectW(NULL, SessionId, LOGONID_CURRENT, L"", TRUE))
        return TRUE;

    while (PhaChoiceDialog(
        WindowHandle,
        L"Connect to session",
        L"Password:",
        NULL,
        0,
        NULL,
        PH_CHOICE_DIALOG_PASSWORD,
        &selectedChoice,
        NULL,
        NULL
        ))
    {
        if (oldSelectedChoice)
        {
            RtlSecureZeroMemory(oldSelectedChoice->Buffer, oldSelectedChoice->Length);
            PhDereferenceObject(oldSelectedChoice);
        }

        oldSelectedChoice = selectedChoice;

        if (WinStationConnectW(NULL, SessionId, LOGONID_CURRENT, selectedChoice->Buffer, TRUE))
        {
            success = TRUE;
            break;
        }
        else
        {
            if (!PhShowContinueStatus(WindowHandle, L"Unable to connect to the session", 0, GetLastError()))
                break;
        }
    }

    if (oldSelectedChoice)
    {
        RtlSecureZeroMemory(oldSelectedChoice->Buffer, oldSelectedChoice->Length);
        PhDereferenceObject(oldSelectedChoice);
    }

    return success;
}

BOOLEAN PhUiDisconnectSession(
    _In_ HWND WindowHandle,
    _In_ ULONG SessionId
    )
{
    if (WinStationDisconnect(NULL, SessionId, FALSE))
        return TRUE;
    else
        PhShowStatus(WindowHandle, L"Unable to disconnect the session", 0, GetLastError());

    return FALSE;
}

BOOLEAN PhUiLogoffSession(
    _In_ HWND WindowHandle,
    _In_ ULONG SessionId
    )
{
    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
        WindowHandle,
        L"logoff",
        L"the user",
        NULL,
        FALSE
        ))
    {
        if (WinStationReset(NULL, SessionId, FALSE))
            return TRUE;
        else
            PhShowStatus(WindowHandle, L"Unable to logoff the session", 0, GetLastError());
    }

    return FALSE;
}

/**
 * Determines if a process is a system process.
 *
 * \param ProcessId The PID of the process to check.
 */
static BOOLEAN PhpIsDangerousProcess(
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    PPH_STRING systemDirectory;
    PPH_STRING fileName;
    PPH_STRING fullName;

    if (ProcessId == SYSTEM_PROCESS_ID)
        return TRUE;

    if (!NT_SUCCESS(status = PhGetProcessImageFileNameByProcessId(ProcessId, &fileName)))
        return FALSE;

    systemDirectory = PH_AUTO(PhGetSystemDirectory());
    PhMoveReference(&fileName, PhGetFileName(fileName));
    PH_AUTO(fileName);

    for (ULONG i = 0; i < RTL_NUMBER_OF(DangerousProcesses); i++)
    {
        fullName = PH_AUTO(PhConcatStringRef3(
            &systemDirectory->sr,
            &PhNtPathSeperatorString,
            &DangerousProcesses[i]
            ));

        if (PhEqualString(fileName, fullName, TRUE))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Checks if the user wants to proceed with an operation.
 *
 * \param WindowHandle A handle to the parent window.
 * \param Verb A verb describing the action.
 * \param Message A message containing additional information
 * about the action.
 * \param WarnOnlyIfDangerous TRUE to skip the confirmation
 * dialog if none of the processes are system processes,
 * FALSE to always show the confirmation dialog.
 * \param Processes An array of pointers to process items.
 * \param NumberOfProcesses The number of process items.
 *
 * \return TRUE if the user wants to proceed with the operation,
 * otherwise FALSE.
 */
static BOOLEAN PhpShowContinueMessageProcesses(
    _In_ HWND WindowHandle,
    _In_ PWSTR Verb,
    _In_opt_ PWSTR Message,
    _In_ BOOLEAN WarnOnlyIfDangerous,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    )
{
    PWSTR object;
    ULONG i;
    BOOLEAN critical = FALSE;
    BOOLEAN dangerous = FALSE;
    BOOLEAN cont = FALSE;

    if (NumberOfProcesses == 0)
        return FALSE;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        HANDLE processHandle;
        BOOLEAN breakOnTermination = FALSE;

        if (PhpIsDangerousProcess(Processes[i]->ProcessId))
        {
            critical = TRUE;
            dangerous = TRUE;
            break;
        }

        if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION, Processes[i]->ProcessId)))
        {
            PhGetProcessBreakOnTermination(processHandle, &breakOnTermination);
            NtClose(processHandle);
        }

        if (breakOnTermination)
        {
            critical = TRUE;
            dangerous = TRUE;
            break;
        }
    }

    if (WarnOnlyIfDangerous && !dangerous)
        return TRUE;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        if (NumberOfProcesses == 1)
        {
            object = Processes[0]->ProcessName->Buffer;
        }
        else if (NumberOfProcesses == 2)
        {
            object = PhaConcatStrings(
                3,
                Processes[0]->ProcessName->Buffer,
                L" and ",
                Processes[1]->ProcessName->Buffer
                )->Buffer;
        }
        else
        {
            object = L"the selected processes";
        }

        if (!dangerous)
        {
            cont = PhShowConfirmMessage(
                WindowHandle,
                Verb,
                object,
                Message,
                FALSE
                );
        }
        else if (!critical)
        {
            cont = PhShowConfirmMessage(
                WindowHandle,
                Verb,
                object,
                PhaConcatStrings(
                3,
                L"You are about to ",
                Verb,
                L" one or more system processes."
                )->Buffer,
                TRUE
                );
        }
        else
        {
            PPH_STRING message;

            if (PhEqualStringZ(Verb, L"terminate", FALSE))
            {
                message = PhaConcatStrings(
                    3,
                    L"You are about to ",
                    Verb,
                    L" one or more critical processes. This will shut down the operating system immediately."
                    );
            }
            else
            {
                message = PhaConcatStrings(
                    3,
                    L"You are about to ",
                    Verb,
                    L" one or more critical processes."
                    );
            }

            cont = PhShowConfirmMessage(
                WindowHandle,
                Verb,
                object,
                message->Buffer,
                TRUE
                );
        }
    }
    else
    {
        cont = TRUE;
    }

    return cont;
}

/**
 * Shows an error message to the user and checks
 * if the user wants to continue.
 *
 * \param WindowHandle A handle to the parent window.
 * \param Verb A verb describing the action which
 * resulted in an error.
 * \param Process The process item which the action
 * was performed on.
 * \param Status A NT status value representing the
 * error.
 * \param Win32Result A Win32 error code representing
 * the error.
 *
 * \return TRUE if the user wants to continue, otherwise
 * FALSE. The result is typically only useful when
 * executing an action on multiple processes.
 */
static BOOLEAN PhpShowErrorProcess(
    _In_ HWND WindowHandle,
    _In_ PWSTR Verb,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    if (!PH_IS_FAKE_PROCESS_ID(Process->ProcessId))
    {
        return PhShowContinueStatus(
            WindowHandle,
            PhaFormatString(
            L"Unable to %s %s (PID %lu)",
            Verb,
            Process->ProcessName->Buffer,
            HandleToUlong(Process->ProcessId)
            )->Buffer,
            Status,
            Win32Result
            );
    }
    else
    {
        return PhShowContinueStatus(
            WindowHandle,
            PhaFormatString(
            L"Unable to %s %s",
            Verb,
            Process->ProcessName->Buffer
            )->Buffer,
            Status,
            Win32Result
            );
    }
}

BOOLEAN PhUiTerminateProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    if (!PhpShowContinueMessageProcesses(
        WindowHandle,
        L"terminate",
        L"Terminating a process will cause unsaved data to be lost.",
        FALSE,
        Processes,
        NumberOfProcesses
        ))
        return FALSE;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        // Note: The current process is a special case (see GH#1770) (dmex)
        if (Processes[i]->ProcessId == NtCurrentProcessId())
        {
            RtlExitUserProcess(STATUS_SUCCESS);
        }

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_TERMINATE,
            Processes[i]->ProcessId
            )))
        {
            // An exit status of 1 is used here for compatibility reasons:
            // 1. Both Task Manager and Process Explorer use 1.
            // 2. winlogon tries to restart explorer.exe if the exit status is not 1.

            status = PhTerminateProcess(processHandle, 1);

            if (status == STATUS_SUCCESS || status == STATUS_PROCESS_IS_TERMINATING)
                PhTerminateProcess(processHandle, DBG_TERMINATE_PROCESS); // debug terminate (dmex)

            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            BOOLEAN connected;

            success = FALSE;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaConcatStrings2(L"Unable to terminate ", Processes[i]->ProcessName->Buffer)->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlProcess(Processes[i]->ProcessId, PhSvcControlProcessTerminate, 0)))
                        success = TRUE;
                    else
                        PhpShowErrorProcess(WindowHandle, L"terminate", Processes[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorProcess(WindowHandle, L"terminate", Processes[i], status, 0))
                    break;
            }
        }
    }

    return success;
}

BOOLEAN PhpUiTerminateTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ PVOID Processes,
    _Inout_ PBOOLEAN Success
    )
{
    NTSTATUS status;
    PSYSTEM_PROCESS_INFORMATION process;
    HANDLE processHandle;
    PPH_PROCESS_ITEM processItem;

    // Note:
    // FALSE should be written to Success if any part of the operation failed.
    // The return value of this function indicates whether to continue with
    // the operation (FALSE if user cancelled).

    // Terminate the process.

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_TERMINATE,
        Process->ProcessId
        )))
    {
        status = PhTerminateProcess(processHandle, 1);

        if (status == STATUS_SUCCESS || status == STATUS_PROCESS_IS_TERMINATING)
            PhTerminateProcess(processHandle, DBG_TERMINATE_PROCESS); // debug terminate (dmex)

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        *Success = FALSE;

        if (!PhpShowErrorProcess(WindowHandle, L"terminate", Process, status, 0))
            return FALSE;
    }

    // Terminate the process' children.

    process = PH_FIRST_PROCESS(Processes);

    do
    {
        if (process->UniqueProcessId != Process->ProcessId &&
            process->InheritedFromUniqueProcessId == Process->ProcessId)
        {
            if (processItem = PhReferenceProcessItem(process->UniqueProcessId))
            {
                if (WindowsVersion >= WINDOWS_10_RS3)
                {
                    // Check the sequence number to make sure it is a descendant.
                    if (processItem->ProcessSequenceNumber >= Process->ProcessSequenceNumber)
                    {
                        if (!PhpUiTerminateTreeProcess(WindowHandle, processItem, Processes, Success))
                        {
                            PhDereferenceObject(processItem);
                            return FALSE;
                        }
                    }
                }
                else
                {
                    // Check the creation time to make sure it is a descendant.
                    if (processItem->CreateTime.QuadPart >= Process->CreateTime.QuadPart)
                    {
                        if (!PhpUiTerminateTreeProcess(WindowHandle, processItem, Processes, Success))
                        {
                            PhDereferenceObject(processItem);
                            return FALSE;
                        }
                    }
                }

                PhDereferenceObject(processItem);
            }
        }
    } while (process = PH_NEXT_PROCESS(process));

    return TRUE;
}

BOOLEAN PhUiTerminateTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    BOOLEAN success = TRUE;
    BOOLEAN cont = FALSE;
    PVOID processes;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            WindowHandle,
            L"terminate",
            PhaConcatStrings2(Process->ProcessName->Buffer, L" and its descendants")->Buffer,
            L"Terminating a process tree will cause the process and its descendants to be terminated.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
    {
        PhShowStatus(WindowHandle, L"Unable to enumerate processes", status, 0);
        return FALSE;
    }

    PhpUiTerminateTreeProcess(WindowHandle, Process, processes, &success);
    PhFree(processes);

    return success;
}

BOOLEAN PhUiSuspendProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    if (!PhpShowContinueMessageProcesses(
        WindowHandle,
        L"suspend",
        NULL,
        TRUE,
        Processes,
        NumberOfProcesses
        ))
        return FALSE;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SUSPEND_RESUME,
            Processes[i]->ProcessId
            )))
        {
            status = NtSuspendProcess(processHandle);
            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            BOOLEAN connected;

            success = FALSE;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaConcatStrings2(L"Unable to suspend ", Processes[i]->ProcessName->Buffer)->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlProcess(Processes[i]->ProcessId, PhSvcControlProcessSuspend, 0)))
                        success = TRUE;
                    else
                        PhpShowErrorProcess(WindowHandle, L"suspend", Processes[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorProcess(WindowHandle, L"suspend", Processes[i], status, 0))
                    break;
            }
        }
    }

    return success;
}

BOOLEAN PhpUiSuspendTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ PVOID Processes,
    _Inout_ PBOOLEAN Success
    )
{
    NTSTATUS status;
    PSYSTEM_PROCESS_INFORMATION process;
    HANDLE processHandle;
    PPH_PROCESS_ITEM processItem;

    // Note:
    // FALSE should be written to Success if any part of the operation failed.
    // The return value of this function indicates whether to continue with
    // the operation (FALSE if user cancelled).

    // Suspend the process.

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_SUSPEND_RESUME,
        Process->ProcessId
        )))
    {
        status = NtSuspendProcess(processHandle);
        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        *Success = FALSE;

        if (!PhpShowErrorProcess(WindowHandle, L"suspend", Process, status, 0))
            return FALSE;
    }

    // Suspend the process' children.

    process = PH_FIRST_PROCESS(Processes);

    do
    {
        if (process->UniqueProcessId != Process->ProcessId &&
            process->InheritedFromUniqueProcessId == Process->ProcessId)
        {
            if (processItem = PhReferenceProcessItem(process->UniqueProcessId))
            {
                if (WindowsVersion >= WINDOWS_10_RS3)
                {
                    // Check the sequence number to make sure it is a descendant.
                    if (processItem->ProcessSequenceNumber >= Process->ProcessSequenceNumber)
                    {
                        if (!PhpUiSuspendTreeProcess(WindowHandle, processItem, Processes, Success))
                        {
                            PhDereferenceObject(processItem);
                            return FALSE;
                        }
                    }
                }
                else
                {
                    // Check the creation time to make sure it is a descendant.
                    if (processItem->CreateTime.QuadPart >= Process->CreateTime.QuadPart)
                    {
                        if (!PhpUiSuspendTreeProcess(WindowHandle, processItem, Processes, Success))
                        {
                            PhDereferenceObject(processItem);
                            return FALSE;
                        }
                    }
                }

                PhDereferenceObject(processItem);
            }
        }
    } while (process = PH_NEXT_PROCESS(process));

    return TRUE;
}

BOOLEAN PhUiSuspendTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    BOOLEAN success = TRUE;
    BOOLEAN cont = FALSE;
    PVOID processes;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            WindowHandle,
            L"suspend",
            PhaConcatStrings2(Process->ProcessName->Buffer, L" and its descendants")->Buffer,
            L"Suspending a process tree will cause the process and its descendants to be suspend.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
    {
        PhShowStatus(WindowHandle, L"Unable to enumerate processes", status, 0);
        return FALSE;
    }

    PhpUiSuspendTreeProcess(WindowHandle, Process, processes, &success);
    PhFree(processes);

    return success;
}

BOOLEAN PhUiResumeProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    if (!PhpShowContinueMessageProcesses(
        WindowHandle,
        L"resume",
        NULL,
        TRUE,
        Processes,
        NumberOfProcesses
        ))
        return FALSE;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SUSPEND_RESUME,
            Processes[i]->ProcessId
            )))
        {
            status = NtResumeProcess(processHandle);
            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            BOOLEAN connected;

            success = FALSE;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaConcatStrings2(L"Unable to resume ", Processes[i]->ProcessName->Buffer)->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlProcess(Processes[i]->ProcessId, PhSvcControlProcessResume, 0)))
                        success = TRUE;
                    else
                        PhpShowErrorProcess(WindowHandle, L"resume", Processes[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorProcess(WindowHandle, L"resume", Processes[i], status, 0))
                    break;
            }
        }
    }

    return success;
}

BOOLEAN PhpUiResumeTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ PVOID Processes,
    _Inout_ PBOOLEAN Success
    )
{
    NTSTATUS status;
    PSYSTEM_PROCESS_INFORMATION process;
    HANDLE processHandle;
    PPH_PROCESS_ITEM processItem;

    // Note:
    // FALSE should be written to Success if any part of the operation failed.
    // The return value of this function indicates whether to continue with
    // the operation (FALSE if user cancelled).

    // Resume the process.

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_SUSPEND_RESUME,
        Process->ProcessId
        )))
    {
        status = NtResumeProcess(processHandle);
        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        *Success = FALSE;

        if (!PhpShowErrorProcess(WindowHandle, L"resume", Process, status, 0))
            return FALSE;
    }

    // Resume the process' children.

    process = PH_FIRST_PROCESS(Processes);

    do
    {
        if (process->UniqueProcessId != Process->ProcessId &&
            process->InheritedFromUniqueProcessId == Process->ProcessId)
        {
            if (processItem = PhReferenceProcessItem(process->UniqueProcessId))
            {
                if (WindowsVersion >= WINDOWS_10_RS3)
                {
                    // Check the sequence number to make sure it is a descendant.
                    if (processItem->ProcessSequenceNumber >= Process->ProcessSequenceNumber)
                    {
                        if (!PhpUiResumeTreeProcess(WindowHandle, processItem, Processes, Success))
                        {
                            PhDereferenceObject(processItem);
                            return FALSE;
                        }
                    }
                }
                else
                {
                    // Check the creation time to make sure it is a descendant.
                    if (processItem->CreateTime.QuadPart >= Process->CreateTime.QuadPart)
                    {
                        if (!PhpUiResumeTreeProcess(WindowHandle, processItem, Processes, Success))
                        {
                            PhDereferenceObject(processItem);
                            return FALSE;
                        }
                    }
                }

                PhDereferenceObject(processItem);
            }
        }
    } while (process = PH_NEXT_PROCESS(process));

    return TRUE;
}

BOOLEAN PhUiResumeTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    BOOLEAN success = TRUE;
    BOOLEAN cont = FALSE;
    PVOID processes;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            WindowHandle,
            L"resume",
            PhaConcatStrings2(Process->ProcessName->Buffer, L" and its descendants")->Buffer,
            L"Resuming a process tree will cause the process and its descendants to be resumed.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
    {
        PhShowStatus(WindowHandle, L"Unable to enumerate processes", status, 0);
        return FALSE;
    }

    PhpUiResumeTreeProcess(WindowHandle, Process, processes, &success);
    PhFree(processes);

    return success;
}

BOOLEAN PhUiFreezeTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            WindowHandle,
            L"freeze",
            Process->ProcessName->Buffer,
            L"Freezing does not persist after exiting System Informer.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    status = PhFreezeProcess(Process->ProcessId);

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"freeze", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiThawTreeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;

    status = PhThawProcess(Process->ProcessId);

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"thaw", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiRestartProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    BOOLEAN tokenIsStronglyNamed = FALSE;
    BOOLEAN tokenIsUIAccessEnabled = FALSE;
    BOOLEAN tokenRevertImpersonation = FALSE;
    HANDLE processHandle = NULL;
    HANDLE newProcessHandle = NULL;
    PPH_STRING fileNameWin32 = NULL;
    PPH_STRING commandLine = NULL;
    PPH_STRING currentDirectory = NULL;
    STARTUPINFOEX startupInfo = { 0 };
    PSECURITY_DESCRIPTOR processSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR tokenSecurityDescriptor = NULL;
    PVOID environment = NULL;
    HANDLE tokenHandle = NULL;
    ULONG flags = 0;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            WindowHandle,
            L"restart",
            Process->ProcessName->Buffer,
            L"The process will be restarted with the same command line, "
            L"working directory and privileges.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    // Fail when restarting the current process otherwise
    // we get terminated before creating the new process. (dmex)
    if (Process->ProcessId == NtCurrentProcessId())
        return FALSE;

    fileNameWin32 = Process->FileName ? PhGetFileName(Process->FileName) : NULL;

    if (PhIsNullOrEmptyString(fileNameWin32) || !PhDoesFileExistWin32(PhGetString(fileNameWin32)))
    {
        status = STATUS_NO_SUCH_FILE;
        goto CleanupExit;
    }

    memset(&startupInfo, 0, sizeof(STARTUPINFOEX));
    startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
    startupInfo.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.StartupInfo.wShowWindow = SW_SHOWNORMAL;

    // Open the process and get the command line and current directory.

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
        Process->ProcessId
        )))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhGetProcessCommandLine(
        processHandle,
        &commandLine
        )))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhGetProcessCurrentDirectory(
        processHandle,
        !!Process->IsWow64,
        &currentDirectory
        )))
    {
        goto CleanupExit;
    }

    NtClose(processHandle);
    processHandle = NULL;

    // Start the process.
    // Use the existing process as the parent of the new process,
    // the new process will inherit everything from the parent process (dmex)

    status = PhOpenProcess(
        &processHandle,
        PROCESS_CREATE_PROCESS | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | (PhGetOwnTokenAttributes().Elevated ? READ_CONTROL : 0),
        Process->ProcessId
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhInitializeProcThreadAttributeList(&startupInfo.lpAttributeList, 1);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhUpdateProcThreadAttribute(
        startupInfo.lpAttributeList,
        PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
        &(HANDLE){ processHandle },
        sizeof(HANDLE)
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhOpenProcessToken(
        processHandle,
        TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY | (PhGetOwnTokenAttributes().Elevated ? READ_CONTROL : 0),
        &tokenHandle
        );

    if (NT_SUCCESS(status))
    {
        status = PhGetTokenUIAccess(tokenHandle, &tokenIsUIAccessEnabled);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }
    else
    {
        status = PhOpenProcessToken(
            processHandle,
            TOKEN_QUERY | (PhGetOwnTokenAttributes().Elevated ? READ_CONTROL : 0),
            &tokenHandle
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    if (PhGetOwnTokenAttributes().Elevated)
    {
        PhGetObjectSecurity(
            processHandle,
            OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
            &processSecurityDescriptor
            );
        PhGetObjectSecurity(
            tokenHandle,
            OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
            &tokenSecurityDescriptor
            );
    }

    if (CreateEnvironmentBlock_Import() && CreateEnvironmentBlock_Import()(&environment, tokenHandle, FALSE))
    {
        flags |= PH_CREATE_PROCESS_UNICODE_ENVIRONMENT;
    }

    if (NT_SUCCESS(PhGetProcessIsStronglyNamed(processHandle, &tokenIsStronglyNamed)) && tokenIsStronglyNamed)
    {
        tokenRevertImpersonation = NT_SUCCESS(PhImpersonateToken(NtCurrentThread(), tokenHandle));
    }

    status = PhCreateProcessWin32Ex(
        PhGetString(fileNameWin32),
        PhGetString(commandLine),
        environment,
        PhGetString(currentDirectory),
        &startupInfo.StartupInfo,
        PH_CREATE_PROCESS_SUSPENDED | PH_CREATE_PROCESS_NEW_CONSOLE | PH_CREATE_PROCESS_EXTENDED_STARTUPINFO | PH_CREATE_PROCESS_DEFAULT_ERROR_MODE | flags,
        tokenHandle,
        NULL,
        &newProcessHandle,
        NULL
        );

    if (!NT_SUCCESS(status)) // Try without the token (dmex)
    {
        status = PhCreateProcessWin32Ex(
            PhGetString(fileNameWin32),
            PhGetString(commandLine),
            environment,
            PhGetString(currentDirectory),
            &startupInfo.StartupInfo,
            PH_CREATE_PROCESS_SUSPENDED | PH_CREATE_PROCESS_NEW_CONSOLE | PH_CREATE_PROCESS_EXTENDED_STARTUPINFO | PH_CREATE_PROCESS_DEFAULT_ERROR_MODE | flags,
            NULL,
            NULL,
            &newProcessHandle,
            NULL
            );
    }

    if (!NT_SUCCESS(status) && tokenIsUIAccessEnabled) // Try UIAccess (dmex)
    {
        status = PhShellExecuteEx(
            WindowHandle,
            PhGetString(fileNameWin32),
            PhGetString(commandLine),
            PhGetString(currentDirectory),
            SW_SHOW,
            PH_SHELL_EXECUTE_DEFAULT,
            0,
            &newProcessHandle
            );
    }

    if (tokenRevertImpersonation)
    {
        PhRevertImpersonationToken(NtCurrentThread());
    }

    if (NT_SUCCESS(status))
    {
        PROCESS_BASIC_INFORMATION basicInfo;

        // See runas.c for a description of the Windows issue with PROC_THREAD_ATTRIBUTE_PARENT_PROCESS
        // requiring the reset of the security descriptor. (dmex)

        if (PhGetOwnTokenAttributes().Elevated && !tokenIsUIAccessEnabled) // Skip processes with UIAccess (dmex)
        {
            HANDLE tokenWriteHandle = NULL;

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
                &tokenWriteHandle
                )))
            {
                PhSetObjectSecurity(
                    tokenWriteHandle,
                    OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
                    tokenSecurityDescriptor
                    );
                NtClose(tokenWriteHandle);
            }
        }

        if (NT_SUCCESS(PhGetProcessBasicInformation(newProcessHandle, &basicInfo)))
        {
            AllowSetForegroundWindow(HandleToUlong(basicInfo.UniqueProcessId));
        }

        // Terminate the existing process.

        PhTerminateProcess(processHandle, 1);

        // Resume the new process.

        NtResumeProcess(newProcessHandle);
    }

CleanupExit:

    if (tokenHandle)
    {
        NtClose(tokenHandle);
    }

    if (newProcessHandle)
    {
        NtClose(newProcessHandle);
    }

    if (processHandle)
    {
        NtClose(processHandle);
    }

    if (startupInfo.lpAttributeList)
    {
        PhDeleteProcThreadAttributeList(startupInfo.lpAttributeList);
    }

    if (environment && DestroyEnvironmentBlock_Import())
    {
        DestroyEnvironmentBlock_Import()(environment);
    }

    if (tokenSecurityDescriptor)
    {
        PhFree(tokenSecurityDescriptor);
    }

    if (processSecurityDescriptor)
    {
        PhFree(processSecurityDescriptor);
    }

    if (currentDirectory)
    {
        PhDereferenceObject(currentDirectory);
    }

    if (commandLine)
    {
        PhDereferenceObject(commandLine);
    }

    if (fileNameWin32)
    {
        PhDereferenceObject(fileNameWin32);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"restart", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

// Contributed by evilpie (#2981421)
BOOLEAN PhUiDebugProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    static PH_STRINGREF aeDebugKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug");
#ifdef _WIN64
    static PH_STRINGREF aeDebugWow64KeyName = PH_STRINGREF_INIT(L"Software\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug");
#endif
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    PPH_STRING debuggerCommand = NULL;
    PH_STRING_BUILDER commandLineBuilder;
    HANDLE keyHandle;
    PPH_STRING debugger;
    PH_STRINGREF commandPart;
    PH_STRINGREF dummy;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            WindowHandle,
            L"debug",
            Process->ProcessName->Buffer,
            L"Debugging a process may result in loss of data.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    status = PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
#ifdef _WIN64
        Process->IsWow64 ? &aeDebugWow64KeyName : &aeDebugKeyName,
#else
        &aeDebugKeyName,
#endif
        0
        );

    if (NT_SUCCESS(status))
    {
        if (debugger = PH_AUTO(PhQueryRegistryStringZ(keyHandle, L"Debugger")))
        {
            if (PhSplitStringRefAtChar(&debugger->sr, L'"', &dummy, &commandPart) &&
                PhSplitStringRefAtChar(&commandPart, L'"', &commandPart, &dummy))
            {
                debuggerCommand = PhCreateString2(&commandPart);
            }
        }

        NtClose(keyHandle);
    }

    if (PhIsNullOrEmptyString(debuggerCommand))
    {
        PhShowStatus(WindowHandle, L"Unable to locate the debugger.", STATUS_OBJECT_NAME_NOT_FOUND, 0);
        return FALSE;
    }

    PhInitializeStringBuilder(&commandLineBuilder, debuggerCommand->Length + 30);
    PhAppendCharStringBuilder(&commandLineBuilder, L'"');
    PhAppendStringBuilder(&commandLineBuilder, &debuggerCommand->sr);
    PhAppendCharStringBuilder(&commandLineBuilder, L'"');
    PhAppendFormatStringBuilder(&commandLineBuilder, L" -p %lu", HandleToUlong(Process->ProcessId));

    status = PhCreateProcessWin32(
        NULL,
        commandLineBuilder.String->Buffer,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        NULL
        );

    PhDeleteStringBuilder(&commandLineBuilder);
    PhDereferenceObject(debuggerCommand);

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"debug", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiReduceWorkingSetProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        status = PhOpenProcess(
            &processHandle,
            PROCESS_CREATE_THREAD,
            Processes[i]->ProcessId
            );

        if ((status == STATUS_ACCESS_DENIED) && (KsiLevel() == KphLevelMax))
        {
            status = PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION, // HACK for KphProcessEmptyWorkingSet (dmex)
                Processes[i]->ProcessId
                );
        }

        if (NT_SUCCESS(status))
        {
            status = PhSetProcessEmptyWorkingSet(processHandle);
            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorProcess(WindowHandle, L"reduce the working set of", Processes[i], status, 0))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiSetVirtualizationProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ BOOLEAN Enable
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    HANDLE processHandle;
    HANDLE tokenHandle;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            WindowHandle,
            L"set",
            L"virtualization for the process",
            L"Enabling or disabling virtualization for a process may "
            L"alter its functionality and produce undesirable effects.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION,
        Process->ProcessId
        )))
    {
        if (NT_SUCCESS(status = PhOpenProcessToken(
            processHandle,
            TOKEN_WRITE,
            &tokenHandle
            )))
        {
            status = PhSetTokenIsVirtualizationEnabled(tokenHandle, Enable);
            NtClose(tokenHandle);
        }

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"set virtualization for", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetCriticalProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    BOOLEAN breakOnTermination;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_SET_INFORMATION,
        Process->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhGetProcessBreakOnTermination(
            processHandle,
            &breakOnTermination
            );

        if (NT_SUCCESS(status))
        {
            if (!breakOnTermination && (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                WindowHandle,
                L"enable",
                L"critical status on the process",
                L"If the process ends, the operating system will shut down immediately.",
                TRUE
                )))
            {
                status = PhSetProcessBreakOnTermination(processHandle, TRUE);
            }
            else if (breakOnTermination && (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                WindowHandle,
                L"disable",
                L"critical status on the process",
                NULL,
                FALSE
                )))
            {
                status = PhSetProcessBreakOnTermination(processHandle, FALSE);
            }
        }

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"set critical status for", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetEcoModeProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    POWER_THROTTLING_PROCESS_STATE powerThrottlingState;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_INFORMATION,
        Process->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        status = PhGetProcessPowerThrottlingState(
            processHandle,
            &powerThrottlingState
            );

        if (NT_SUCCESS(status))
        {
            if (!(
                powerThrottlingState.ControlMask & POWER_THROTTLING_PROCESS_EXECUTION_SPEED &&
                powerThrottlingState.StateMask & POWER_THROTTLING_PROCESS_EXECUTION_SPEED
                ))
            {
                if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                    WindowHandle,
                    L"enable",
                    L"Eco mode for this process",
                    L"Eco mode will lower process priority and improve power efficiency but may cause instability in some processes.",
                    FALSE
                    ))
                {
                    // Taskmgr sets the process priority to idle before enabling 'Eco mode'. (dmex)
                    PhSetProcessPriority(processHandle, PROCESS_PRIORITY_CLASS_IDLE);

                    status = PhSetProcessPowerThrottlingState(
                        processHandle,
                        POWER_THROTTLING_PROCESS_EXECUTION_SPEED,
                        POWER_THROTTLING_PROCESS_EXECUTION_SPEED
                        );
                }
            }
            else
            {
                //if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                //    WindowHandle,
                //    L"disable",
                //    L"Eco mode for this process",
                //    L"Eco mode will lower process priority and improve power efficiency but may cause instability in some processes.",
                //    FALSE
                //    ))
                {
                    // Taskmgr does not properly restore the original priority after it has exited
                    // and you later decide to disable 'Eco mode', so we'll restore normal priority
                    // which isn't quite correct but still way better than what taskmgr does. (dmex)
                    PhSetProcessPriority(processHandle, PROCESS_PRIORITY_CLASS_NORMAL);

                    status = PhSetProcessPowerThrottlingState(processHandle, 0, 0);
                }
            }
        }

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"set Eco mode for", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetExecutionRequiredProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
)
{
    NTSTATUS status;

    if (PhIsProcessExecutionRequired(Process->ProcessId))
    {
        status = PhProcessExecutionRequiredDisable(Process->ProcessId);
    }
    else
    {
        status = PhProcessExecutionRequiredEnable(Process->ProcessId);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"create PLM power request for", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiDetachFromDebuggerProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE debugObjectHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME,
        Process->ProcessId
        )))
    {
        if (NT_SUCCESS(status = PhGetProcessDebugObject(
            processHandle,
            &debugObjectHandle
            )))
        {
            // Disable kill-on-close.
            if (NT_SUCCESS(status = PhSetDebugKillProcessOnExit(
                debugObjectHandle,
                FALSE
                )))
            {
                status = NtRemoveProcessDebug(processHandle, debugObjectHandle);
            }

            NtClose(debugObjectHandle);
        }

        NtClose(processHandle);
    }

    if (status == STATUS_PORT_NOT_SET)
    {
        PhShowInformation2(WindowHandle, L"The process is not being debugged.", L"%s", L"");
        return FALSE;
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"detach debugger from", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiLoadDllProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"DLL files (*.dll)", L"*.dll" },
        { L"All files (*.*)", L"*.*" }
    };

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE processHandle = NULL;
    PVOID fileDialog;
    PPH_STRING fileName;

    fileDialog = PhCreateOpenFileDialog();
    PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_DONTADDTORECENT);
    PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));

    if (!PhShowFileDialog(WindowHandle, fileDialog))
    {
        PhFreeFileDialog(fileDialog);
        return FALSE;
    }

    fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
    PhFreeFileDialog(fileDialog);

    // Windows 8 requires ALL_ACCESS for PLM execution requests. (dmex)
    if (WindowsVersion >= WINDOWS_8 && WindowsVersion <= WINDOWS_8_1)
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_ALL_ACCESS,
            Process->ProcessId
            );
    }

    // Windows 10 and above require SET_LIMITED for PLM execution requests. (dmex)
    if (!NT_SUCCESS(status))
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_LIMITED_INFORMATION |
            PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
            PROCESS_VM_READ | PROCESS_VM_WRITE | SYNCHRONIZE,
            Process->ProcessId
            );
    }

    if (NT_SUCCESS(status))
    {
        LARGE_INTEGER timeout;

        timeout.QuadPart = -(LONGLONG)UInt32x32To64(5, PH_TIMEOUT_SEC);
        status = PhLoadDllProcess(processHandle, &fileName->sr, &timeout);

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"load the DLL into", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetIoPriorityProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses,
    _In_ IO_PRIORITY_HINT IoPriority
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SET_INFORMATION,
            Processes[i]->ProcessId
            )))
        {
            if (Processes[i]->ProcessId != SYSTEM_PROCESS_ID)
            {
                status = PhSetProcessIoPriority(processHandle, IoPriority);
            }
            else
            {
                // See comment in PhUiSetPriorityProcesses.
                status = STATUS_UNSUCCESSFUL;
            }

            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            BOOLEAN connected;

            success = FALSE;

            // The operation may have failed due to the lack of SeIncreaseBasePriorityPrivilege.
            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaConcatStrings2(L"Unable to set the I/O priority of ", Processes[i]->ProcessName->Buffer)->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlProcess(Processes[i]->ProcessId, PhSvcControlProcessIoPriority, IoPriority)))
                        success = TRUE;
                    else
                        PhpShowErrorProcess(WindowHandle, L"set the I/O priority of", Processes[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorProcess(WindowHandle, L"set the I/O priority of", Processes[i], status, 0))
                    break;
            }
        }
    }

    return success;
}

BOOLEAN PhUiSetPagePriorityProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ ULONG PagePriority
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        Process->ProcessId
        )))
    {
        if (Process->ProcessId != SYSTEM_PROCESS_ID)
        {
            status = PhSetProcessPagePriority(processHandle, PagePriority);
        }
        else
        {
            // See comment in PhUiSetPriorityProcesses.
            status = STATUS_UNSUCCESSFUL;
        }

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"set the page priority of", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetPriorityProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses,
    _In_ ULONG PriorityClass
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SET_INFORMATION,
            Processes[i]->ProcessId
            )))
        {
            if (Processes[i]->ProcessId != SYSTEM_PROCESS_ID)
            {
                status = PhSetProcessPriority(processHandle, (UCHAR)PriorityClass);
            }
            else
            {
                // Changing the priority of System can lead to a BSOD on some versions of Windows,
                // so disallow this.
                status = STATUS_UNSUCCESSFUL;
            }

            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            BOOLEAN connected;

            success = FALSE;

            // The operation may have failed due to the lack of SeIncreaseBasePriorityPrivilege.
            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaConcatStrings2(L"Unable to set the priority of ", Processes[i]->ProcessName->Buffer)->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlProcess(Processes[i]->ProcessId, PhSvcControlProcessPriority, PriorityClass)))
                        success = TRUE;
                    else
                        PhpShowErrorProcess(WindowHandle, L"set the priority of", Processes[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorProcess(WindowHandle, L"set the priority of", Processes[i], status, 0))
                    break;
            }
        }
    }

    return success;
}

BOOLEAN PhUiSetBoostPriorityProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM* Processes,
    _In_ ULONG NumberOfProcesses,
    _In_ BOOLEAN PriorityBoost
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_SET_INFORMATION,
            Processes[i]->ProcessId
            )))
        {
            status = PhSetProcessPriorityBoost(processHandle, PriorityBoost);
            NtClose(processHandle);

            if (!NT_SUCCESS(status))
            {
                success = FALSE;

                if (!PhpShowErrorProcess(WindowHandle, L"change boost priority of", Processes[i], status, 0))
                    break;
            }
        }
    }

    return success;
}

BOOLEAN PhUiSetBoostPriorityProcess(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM Process,
    _In_ BOOLEAN PriorityBoost
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION,
        Process->ProcessId
        )))
    {
        status = PhSetProcessPriorityBoost(processHandle, PriorityBoost);
        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorProcess(WindowHandle, L"set the boost priority of", Process, status, 0);
        return FALSE;
    }

    return TRUE;
}

#pragma region Service Progress Dialog
FORCEINLINE
VOID
TaskDialog_NavigatePage(
    _In_ HWND WindowHandle,
    _In_ TASKDIALOGCONFIG* Config)
{
    assert(HandleToUlong(NtCurrentThreadId()) == GetWindowThreadProcessId(WindowHandle, NULL));

    SendMessage(WindowHandle, TDM_NAVIGATE_PAGE, 0, (LPARAM)Config);
}

typedef struct _PH_UI_SERVICE_PROGRESS_DIALOG
{
    HWND WindowHandle;
    HWND ParentWindowHandle;

    PCWSTR Object;
    PCWSTR Verb;
    PCWSTR Message;

    PPH_STRING StatusMessage;
    PPH_STRING StatusContent;

    PPH_LIST ServiceItemList;

    volatile LONG RequireElevation;
    struct
    {
        BOOLEAN Flags;
        union
        {
            BOOLEAN Warning : 1;
            BOOLEAN Spare : 7;
        };
    };

    WNDPROC OldWndProc;
    PUSER_THREAD_START_ROUTINE ActionCallback;
    PHSVC_API_CONTROLSERVICE_COMMAND ActionCommand;
} PH_UI_SERVICE_PROGRESS_DIALOG, *PPH_UI_SERVICE_PROGRESS_DIALOG;

typedef struct _PH_UI_SERVICE_ITEM
{
    NTSTATUS Status;
    PPH_SERVICE_ITEM Service;
} PH_UI_SERVICE_ITEM, *PPH_UI_SERVICE_ITEM;

#define WM_PHSVC_ERROR (WM_APP + 1)
#define WM_PHSVC_EXIT (WM_APP + 2)

VOID PhUiNavigateServiceProgressDialogPage(
    _In_ PPH_UI_SERVICE_PROGRESS_DIALOG Context
    );
#pragma endregion

HRESULT CALLBACK PhpUiServiceErrorDialogCallbackProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR Context
    )
{
    PPH_UI_SERVICE_PROGRESS_DIALOG context = (PPH_UI_SERVICE_PROGRESS_DIALOG)Context;

    switch (WindowMessage)
    {
    case TDN_NAVIGATED:
        {
            if (InterlockedCompareExchange(&context->RequireElevation, FALSE, FALSE))
            {
                SendMessage(WindowHandle, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDYES, TRUE);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            ULONG buttonId = (ULONG)wParam;

            if (buttonId == IDYES)
            {
                PhUiNavigateServiceProgressDialogPage(context);
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID PhUiNavigateServiceErrorDialogPage(
    _In_ PPH_UI_SERVICE_PROGRESS_DIALOG Context,
    _In_ PPH_STRING MainInstruction,
    _In_opt_ PPH_STRING MainContent
    )
{
    TASKDIALOGCONFIG config;
    TASKDIALOG_BUTTON buttons[2];

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = TD_ERROR_ICON;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = PhpUiServiceErrorDialogCallbackProc;
    config.pszMainInstruction = PhGetString(MainInstruction);
    if (MainContent) config.pszContent = PhGetString(MainContent);
    config.cxWidth = 200;

    buttons[0].nButtonID = IDYES;
    buttons[0].pszButtonText = L"Continue";
    buttons[1].nButtonID = IDNO;
    buttons[1].pszButtonText = L"Cancel";

    if (InterlockedCompareExchange(&Context->RequireElevation, FALSE, FALSE))
    {
        config.cButtons = 2;
        config.pButtons = buttons;
        config.nDefaultButton = IDYES;
    }

    TaskDialog_NavigatePage(Context->WindowHandle, &config);
}

VOID PhUiNavigateServiceCompleteDialogPage(
    _In_ PPH_UI_SERVICE_PROGRESS_DIALOG Context
    )
{
    PostMessage(Context->WindowHandle, WM_PHSVC_EXIT, 0, 0);
}

VOID PhUiNavigateServiceErrorDialogPageFromThread(
    _In_ PPH_UI_SERVICE_PROGRESS_DIALOG Context
    )
{
    PostMessage(Context->WindowHandle, WM_PHSVC_ERROR, 0, 0);
}

NTSTATUS PhpUiServicePendingStartCallback(
    _In_ PPH_UI_SERVICE_PROGRESS_DIALOG Context
    )
{
    PPH_LIST serviceErrorList = PhCreateList(1);

    if (InterlockedCompareExchange(&Context->RequireElevation, FALSE, FALSE))
    {
        NTSTATUS status;
        BOOLEAN connected;

        if (PhpElevationLevelAndConnectToPhSvc(Context->WindowHandle, &connected) && connected)
        {
            for (ULONG i = 0; i < Context->ServiceItemList->Count; i++)
            {
                PPH_SERVICE_ITEM serviceItem = Context->ServiceItemList->Items[i];

                status = PhSvcCallControlService(
                    PhGetString(serviceItem->Name),
                    Context->ActionCommand
                    );

                if (NT_SUCCESS(status))
                {
                    ULONG index;

                    index = PhFindItemList(Context->ServiceItemList, serviceItem);

                    if (index != ULONG_MAX)
                    {
                        PhRemoveItemList(Context->ServiceItemList, index);
                        PhDereferenceObject(serviceItem);
                    }
                }
                else
                {
                    PhAddItemList(serviceErrorList, LongToPtr(status));
                }
            }

            PhUiDisconnectFromPhSvc();
        }
        else
        {
            PhUiNavigateServiceCompleteDialogPage(Context);
            goto CleanupExit;
        }
    }
    else
    {
        for (ULONG i = 0; i < Context->ServiceItemList->Count; i++)
        {
            PPH_SERVICE_ITEM serviceItem = Context->ServiceItemList->Items[i];
            NTSTATUS status;

            status = Context->ActionCallback(serviceItem);

            if (NT_SUCCESS(status))
            {
                ULONG index;

                index = PhFindItemList(Context->ServiceItemList, serviceItem);

                if (index != ULONG_MAX)
                {
                    PhRemoveItemList(Context->ServiceItemList, index);
                    PhDereferenceObject(serviceItem);
                }
            }
            else
            {
                PhAddItemList(serviceErrorList, LongToPtr(status));
            }
        }
    }

    InterlockedExchange(&Context->RequireElevation, FALSE);

    if (serviceErrorList->Count)
    {
        for (ULONG i = 0; i < serviceErrorList->Count; i++)
        {
            NTSTATUS status = PtrToLong(serviceErrorList->Items[i]);

            if (status == STATUS_ACCESS_DENIED || status == STATUS_PRIVILEGE_NOT_HELD)
            {
                InterlockedExchange(&Context->RequireElevation, TRUE);
                break;
            }
        }
    }

    if (Context->ServiceItemList->Count)
    {
        PH_STRING_BUILDER stringBuilder;

        PhInitializeStringBuilder(&stringBuilder, 0x50);

        for (ULONG i = 0; i < Context->ServiceItemList->Count; i++)
        {
            PPH_STRING serviceName = NULL;
            PPH_STRING statusMessage = NULL;

            if (!PhIsNullOrEmptyString(((PPH_SERVICE_ITEM)Context->ServiceItemList->Items[i])->Name))
            {
                serviceName = ((PPH_SERVICE_ITEM)Context->ServiceItemList->Items[i])->Name;
            }

            if (i < serviceErrorList->Count)
            {
                statusMessage = PhGetStatusMessage(PtrToLong(serviceErrorList->Items[i]), 0);
            }

            if (!PhIsNullOrEmptyString(serviceName))
                PhAppendStringBuilder(&stringBuilder, &serviceName->sr);
            PhAppendStringBuilder2(&stringBuilder, L": ");

            PhAppendFormatStringBuilder(&stringBuilder, L"(0x%lx) ", PtrToLong(serviceErrorList->Items[i]));

            if (!PhIsNullOrEmptyString(statusMessage))
            {
                PhAppendStringBuilder(&stringBuilder, &statusMessage->sr);
                PhClearReference(&statusMessage);
            }

            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
        }

        if (stringBuilder.String->Length != 0)
        {
            PhRemoveEndStringBuilder(&stringBuilder, 2);
        }

        if (InterlockedCompareExchange(&Context->RequireElevation, FALSE, FALSE))
        {
            PhAppendStringBuilder2(&stringBuilder, L"\r\n\r\n");
            PhAppendStringBuilder2(&stringBuilder, L"You will need to provide administrator permission. "
                L"Click Continue to complete this operation.");
        }

        {
            PPH_STRING message;
            PPH_STRING content;

            message = PhFormatString(L"Unable to %s services:", Context->Verb);
            content = PhFinalStringBuilderString(&stringBuilder);

            InterlockedExchangePointer(&Context->StatusMessage, message);
            InterlockedExchangePointer(&Context->StatusContent, content);

            PhUiNavigateServiceErrorDialogPageFromThread(Context);
        }
    }
    else
    {
        PhUiNavigateServiceCompleteDialogPage(Context);
    }

CleanupExit:
    PhClearReference(&serviceErrorList);

    PhDereferenceObject(Context);

    return STATUS_SUCCESS;
}

HRESULT CALLBACK PhpUiServiceProgressDialogCallbackProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR Context
    )
{
    PPH_UI_SERVICE_PROGRESS_DIALOG context = (PPH_UI_SERVICE_PROGRESS_DIALOG)Context;

    switch (WindowMessage)
    {
    case TDN_NAVIGATED:
        {
            SendMessage(WindowHandle, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(WindowHandle, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            PhReferenceObject(context);
            PhCreateThread2(PhpUiServicePendingStartCallback, context);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            ULONG buttonId = (ULONG)wParam;

            if (buttonId != IDCANCEL)
            {
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID PhUiNavigateServiceProgressDialogPage(
    _In_ PPH_UI_SERVICE_PROGRESS_DIALOG Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CAN_BE_MINIMIZED;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = TD_INFORMATION_ICON;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = PhpUiServiceProgressDialogCallbackProc;
    config.pszMainInstruction = PhaConcatStrings(5, L"Attempting to ", Context->Verb, L" ", Context->Object, L"...")->Buffer;
    config.cxWidth = 200;

    TaskDialog_NavigatePage(Context->WindowHandle, &config);
}

HRESULT CALLBACK PhpUiServiceConfirmDialogCallbackProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR Context
    )
{
    PPH_UI_SERVICE_PROGRESS_DIALOG context = (PPH_UI_SERVICE_PROGRESS_DIALOG)Context;

    switch (WindowMessage)
    {
    case TDN_BUTTON_CLICKED:
        {
            ULONG buttonId = (ULONG)wParam;

            if (buttonId == IDYES)
            {
                PhUiNavigateServiceProgressDialogPage(context);
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

VOID PhpShowServiceProgressInitializeText(
    _In_ PPH_UI_SERVICE_PROGRESS_DIALOG Context,
    _Out_ PPH_STRING* Verb,
    _Out_ PPH_STRING* VerbCaps,
    _Out_ PPH_STRING* Action
    )
{
    if (Context->ServiceItemList->Count == 1)
        Context->Object = L"the selected service";
    else
        Context->Object = L"the selected services";

    // Make sure the verb is all lowercase.
    *Verb = PhaLowerString(PhaCreateString((PWSTR)Context->Verb));

    // "terminate" -> "Terminate"
    *VerbCaps = PhaDuplicateString(*Verb);
    if (!PhIsNullOrEmptyString(*VerbCaps)) (*VerbCaps)->Buffer[0] = PhUpcaseUnicodeChar((*VerbCaps)->Buffer[0]);

    // "terminate", "the process" -> "terminate the process"
    *Action = PhaConcatStrings(3, (*Verb)->Buffer, L" ", Context->Object);
}

VOID PhShowServiceProgressDialogConfirmMessage(
    _In_ PPH_UI_SERVICE_PROGRESS_DIALOG Context
    )
{
    TASKDIALOGCONFIG config;
    TASKDIALOG_BUTTON buttons[2];
    PPH_STRING verb;
    PPH_STRING verbCaps;
    PPH_STRING action;

    PhpShowServiceProgressInitializeText(Context, &verb, &verbCaps, &action);

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.pszWindowTitle = PhApplicationName;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = PhpUiServiceConfirmDialogCallbackProc;
    config.pszMainIcon = Context->Warning ? TD_WARNING_ICON : TD_INFORMATION_ICON;
    config.pszMainInstruction = PhaConcatStrings(3, L"Do you want to ", action->Buffer, L"?")->Buffer;
    if (Context->Message) config.pszContent = PhaConcatStrings2((PWSTR)Context->Message, L" Are you sure you want to continue?")->Buffer;

    buttons[0].nButtonID = IDYES;
    buttons[0].pszButtonText = verbCaps->Buffer;
    buttons[1].nButtonID = IDNO;
    buttons[1].pszButtonText = L"Cancel";

    config.cButtons = 2;
    config.pButtons = buttons;
    config.nDefaultButton = IDYES;
    config.cxWidth = 200;

    TaskDialog_NavigatePage(Context->WindowHandle, &config);
}

static LRESULT CALLBACK PhpUiServiceProgressDialogWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_UI_SERVICE_PROGRESS_DIALOG context;
    WNDPROC oldWndProc;

    context = PhGetWindowContext(WindowHandle, MAXCHAR);

    if (!context)
        goto DefaultWndProc;

    oldWndProc = context->OldWndProc;

    switch (WindowMessage)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(WindowHandle, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(WindowHandle, MAXCHAR);
        }
        break;
    case WM_PHSVC_ERROR:
        {
            PPH_STRING message;
            PPH_STRING content;

            message = InterlockedExchangePointer(&context->StatusMessage, NULL);
            content = InterlockedExchangePointer(&context->StatusContent, NULL);

            PhUiNavigateServiceErrorDialogPage(
                context,
                message,
                content
                );

            PhClearReference(&message);
            PhClearReference(&content);
        }
        goto DefaultWndProc;
    case WM_PHSVC_EXIT:
        {
            CallWindowProc(oldWndProc, WindowHandle, TDM_CLICK_BUTTON, IDCANCEL, 0);
        }
        goto DefaultWndProc;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

HRESULT CALLBACK PhpUiServiceInitializeDialogCallbackProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR Context
    )
{
    PPH_UI_SERVICE_PROGRESS_DIALOG context = (PPH_UI_SERVICE_PROGRESS_DIALOG)Context;

    switch (WindowMessage)
    {
    case TDN_CREATED:
        {
            context->WindowHandle = WindowHandle;

            PhSetApplicationWindowIcon(WindowHandle);

            PhCenterWindow(WindowHandle, context->ParentWindowHandle);

            context->OldWndProc = PhGetWindowProcedure(WindowHandle);
            PhSetWindowContext(WindowHandle, MAXCHAR, context);
            PhSetWindowProcedure(WindowHandle, PhpUiServiceProgressDialogWndProc);

            PhShowServiceProgressDialogConfirmMessage(context);
        }
        break;
    }

    return S_OK;
}

NTSTATUS PhShowServiceProgressDialogThread(
    _In_ PPH_UI_SERVICE_PROGRESS_DIALOG Context
    )
{
    PH_AUTO_POOL autoPool;
    TASKDIALOGCONFIG config;

    PhInitializeAutoPool(&autoPool);

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.pfCallback = PhpUiServiceInitializeDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pszContent = L"Initializing...";
    config.cxWidth = 200;

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    PhDeleteAutoPool(&autoPool);
    PhDereferenceObject(Context);

    return STATUS_SUCCESS;
}

static VOID PhServiceProgressContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_UI_SERVICE_PROGRESS_DIALOG context = Object;

    PhDereferenceObjects(context->ServiceItemList->Items, context->ServiceItemList->Count);
    PhDereferenceObject(context->ServiceItemList);
}

PPH_UI_SERVICE_PROGRESS_DIALOG PhCreateServiceProgressContext(
    VOID
    )
{
    static PPH_OBJECT_TYPE PhServiceProgressObjectType = NULL;
    static PH_INITONCE PhServiceProgressTypeInitOnce = PH_INITONCE_INIT;
    PPH_UI_SERVICE_PROGRESS_DIALOG context;

    if (PhBeginInitOnce(&PhServiceProgressTypeInitOnce))
    {
        PhServiceProgressObjectType = PhCreateObjectType(L"ServiceProgressObjectType", 0, PhServiceProgressContextDeleteProcedure);
        PhEndInitOnce(&PhServiceProgressTypeInitOnce);
    }

    context = PhCreateObject(sizeof(PH_UI_SERVICE_PROGRESS_DIALOG), PhServiceProgressObjectType);
    memset(context, 0, sizeof(PH_UI_SERVICE_PROGRESS_DIALOG));

    return context;
}

VOID PhShowServiceProgressDialog(
    _In_ HWND WindowHandle,
    _In_ PCWSTR Verb,
    _In_ PCWSTR Message,
    _In_ BOOLEAN Warning,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices,
    _In_ PUSER_THREAD_START_ROUTINE ActionCallback,
    _In_ PHSVC_API_CONTROLSERVICE_COMMAND ActionCommand
    )
{
    PPH_UI_SERVICE_PROGRESS_DIALOG context;

    if (NumberOfServices == 0)
        return;

    PhReferenceObjects(Services, NumberOfServices);

    context = PhCreateServiceProgressContext();
    context->ParentWindowHandle = WindowHandle;
    context->Verb = Verb;
    context->Message = Message;
    context->Warning = Warning;
    context->ActionCallback = ActionCallback;
    context->ActionCommand = ActionCommand;
    context->ServiceItemList = PhCreateList(NumberOfServices);
    PhAddItemsList(context->ServiceItemList, Services, NumberOfServices);

    PhCreateThread2(PhShowServiceProgressDialogThread, context);
}

static BOOLEAN PhpShowContinueMessageServices(
    _In_ HWND WindowHandle,
    _In_ PWSTR Verb,
    _In_ PWSTR Message,
    _In_ BOOLEAN Warning,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    )
{
    PWSTR object;
    BOOLEAN cont = FALSE;

    if (NumberOfServices == 0)
        return FALSE;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        if (NumberOfServices == 1)
        {
            object = L"the selected service";
        }
        else
        {
            object = L"the selected services";
        }

        cont = PhShowConfirmMessage(
            WindowHandle,
            Verb,
            object,
            Message,
            Warning
            );
    }
    else
    {
        cont = TRUE;
    }

    return cont;
}

static NTSTATUS PhpCheckServiceStatus(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG CurrentState,
    _In_ ULONG WaitForState)
{
    NTSTATUS status;
    SERVICE_STATUS_PROCESS serviceStatus;
    ULONG checkpoint;

    status = PhQueryServiceStatus(ServiceHandle, &serviceStatus);

    if (!NT_SUCCESS(status))
        return status;

    if (serviceStatus.dwCurrentState == WaitForState)
        return STATUS_SUCCESS;

    checkpoint = serviceStatus.dwCheckPoint;

    while (
        serviceStatus.dwCurrentState == SERVICE_START_PENDING ||
        serviceStatus.dwCurrentState == SERVICE_STOP_PENDING ||
        serviceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING ||
        serviceStatus.dwCurrentState == SERVICE_PAUSE_PENDING
        )
    {
        PhDelayExecution(serviceStatus.dwWaitHint);

        status = PhQueryServiceStatus(ServiceHandle, &serviceStatus);

        if (!NT_SUCCESS(status))
            return status;

        if (serviceStatus.dwCurrentState == WaitForState)
            return STATUS_SUCCESS;

        if (checkpoint == serviceStatus.dwCheckPoint && (
            serviceStatus.dwCurrentState == SERVICE_START_PENDING ||
            serviceStatus.dwCurrentState == SERVICE_STOP_PENDING ||
            serviceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING ||
            serviceStatus.dwCurrentState == SERVICE_PAUSE_PENDING
            ))
        {

        }

        checkpoint = serviceStatus.dwCheckPoint;
    }

    return status;
}

static BOOLEAN PhpShowErrorService(
    _In_ HWND WindowHandle,
    _In_ PWSTR Verb,
    _In_ PPH_SERVICE_ITEM Service,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    return PhShowContinueStatus(
        WindowHandle,
        PhaFormatString(
        L"Unable to %s %s.",
        Verb,
        Service->Name->Buffer
        )->Buffer,
        Status,
        Win32Result
        );
}

static NTSTATUS PhUiServiceStartCallback(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    NTSTATUS status;
    SC_HANDLE serviceHandle;

    status = PhOpenService(
        &serviceHandle,
        SERVICE_START,
        PhGetString(ServiceItem->Name)
        );

    if (NT_SUCCESS(status))
    {
        status = PhStartService(serviceHandle, 0, NULL);

        PhCloseServiceHandle(serviceHandle);
    }

    return status;
}

BOOLEAN PhUiStartServices(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    if (PhGetIntegerSetting(L"EnableServiceProgressDialog"))
    {
        PhShowServiceProgressDialog(
            WindowHandle,
            L"start",
            L"Starting a service might prevent the system from functioning properly.",
            FALSE,
            Services,
            NumberOfServices,
            PhUiServiceStartCallback,
            PhSvcControlServiceStart
            );
        return FALSE;
    }

    if (!PhpShowContinueMessageServices(
        WindowHandle,
        L"start",
        L"Starting a service might prevent the system from functioning properly.",
        FALSE,
        Services,
        NumberOfServices
        ))
        return FALSE;

    for (i = 0; i < NumberOfServices; i++)
    {
        NTSTATUS status;
        SC_HANDLE serviceHandle;

        success = FALSE;
        status = PhOpenService(&serviceHandle, SERVICE_START, PhGetString(Services[i]->Name));

        if (NT_SUCCESS(status))
        {
            status = PhStartService(serviceHandle, 0, NULL);

            if (NT_SUCCESS(status))
                success = TRUE;

            PhCloseServiceHandle(serviceHandle);
        }

        if (!success)
        {
            BOOLEAN connected;

            success = FALSE;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaConcatStrings2(L"Unable to start ", PhGetString(Services[i]->Name))->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlService(PhGetString(Services[i]->Name), PhSvcControlServiceStart)))
                        success = TRUE;
                    else
                        PhpShowErrorService(WindowHandle, L"start", Services[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorService(WindowHandle, L"start", Services[i], status, 0))
                    break;
            }
        }
    }

    return success;

    //BOOLEAN result = TRUE;
    //ULONG i;
    //
    //for (i = 0; i < NumberOfServices; i++)
    //{
    //    SC_HANDLE serviceHandle;
    //    BOOLEAN success = FALSE;
    //
    //    serviceHandle = PhOpenService(PhGetString(Services[i]->Name), SERVICE_START);
    //
    //    if (serviceHandle)
    //    {
    //        if (StartService(serviceHandle, 0, NULL))
    //            success = TRUE;
    //
    //        PhCloseServiceHandle(serviceHandle);
    //    }
    //
    //    if (!success)
    //    {
    //        NTSTATUS status;
    //
    //        status = PhGetLastWin32ErrorAsNtStatus();
    //        result = FALSE;
    //
    //        PhpShowErrorService(WindowHandle, L"start", Services[i], status, 0);
    //    }
    //}
    //
    //return result;
}

BOOLEAN PhUiStartService(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM Service
    )
{
    SC_HANDLE serviceHandle;
    NTSTATUS status;
    BOOLEAN success = FALSE;

    status = PhOpenService(&serviceHandle, SERVICE_START, PhGetString(Service->Name));

    if (NT_SUCCESS(status))
    {
        status = PhStartService(serviceHandle, 0, NULL);

        if (NT_SUCCESS(status))
            success = TRUE;

        PhCloseServiceHandle(serviceHandle);
    }

    if (!success)
    {
        BOOLEAN connected;

        if (PhpShowErrorAndConnectToPhSvc(
            WindowHandle,
            PhaConcatStrings2(L"Unable to start ", PhGetString(Service->Name))->Buffer,
            status,
            &connected
            ))
        {
            if (connected)
            {
                if (NT_SUCCESS(status = PhSvcCallControlService(PhGetString(Service->Name), PhSvcControlServiceStart)))
                    success = TRUE;
                else
                    PhpShowErrorService(WindowHandle, L"start", Service, status, 0);

                PhUiDisconnectFromPhSvc();
            }
        }
        else
        {
            PhpShowErrorService(WindowHandle, L"start", Service, status, 0);
        }
    }

    return success;
}

static NTSTATUS PhUiServiceContinueCallback(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    NTSTATUS status;
    SC_HANDLE serviceHandle;

    status = PhOpenService(
        &serviceHandle,
        SERVICE_PAUSE_CONTINUE,
        PhGetString(ServiceItem->Name)
        );

    if (NT_SUCCESS(status))
    {
        status = PhContinueService(serviceHandle);

        PhCloseServiceHandle(serviceHandle);
    }

    return status;
}

BOOLEAN PhUiContinueServices(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    if (PhGetIntegerSetting(L"EnableServiceProgressDialog"))
    {
        PhShowServiceProgressDialog(
            WindowHandle,
            L"continue",
            L"Continuing a service might prevent the system from functioning properly.",
            FALSE,
            Services,
            NumberOfServices,
            PhUiServiceContinueCallback,
            PhSvcControlServiceContinue
            );
        return FALSE;
    }

    if (!PhpShowContinueMessageServices(
        WindowHandle,
        L"continue",
        L"Continuing a service might prevent the system from functioning properly.",
        FALSE,
        Services,
        NumberOfServices
        ))
        return FALSE;

    for (i = 0; i < NumberOfServices; i++)
    {
        NTSTATUS status;
        SC_HANDLE serviceHandle;

        success = FALSE;
        status = PhOpenService(&serviceHandle, SERVICE_PAUSE_CONTINUE, PhGetString(Services[i]->Name));

        if (NT_SUCCESS(status))
        {
            status = PhContinueService(serviceHandle);

            if (NT_SUCCESS(status))
                success = TRUE;

            PhCloseServiceHandle(serviceHandle);
        }

        if (!success)
        {
            BOOLEAN connected;

            success = FALSE;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaConcatStrings2(L"Unable to continue ", PhGetString(Services[i]->Name))->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlService(PhGetString(Services[i]->Name), PhSvcControlServiceContinue)))
                        success = TRUE;
                    else
                        PhpShowErrorService(WindowHandle, L"continue", Services[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorService(WindowHandle, L"continue", Services[i], status, 0))
                    break;
            }
        }
    }

    return success;

    //BOOLEAN result = TRUE;
    //ULONG i;
    //
    //for (i = 0; i < NumberOfServices; i++)
    //{
    //    SC_HANDLE serviceHandle;
    //    BOOLEAN success = FALSE;
    //
    //    serviceHandle = PhOpenService(PhGetString(Services[i]->Name), SERVICE_PAUSE_CONTINUE);
    //
    //    if (serviceHandle)
    //    {
    //        SERVICE_STATUS serviceStatus;
    //
    //        if (ControlService(serviceHandle, SERVICE_CONTROL_CONTINUE, &serviceStatus))
    //            success = TRUE;
    //
    //        PhCloseServiceHandle(serviceHandle);
    //    }
    //
    //    if (!success)
    //    {
    //        NTSTATUS status;
    //
    //        status = PhGetLastWin32ErrorAsNtStatus();
    //        result = FALSE;
    //
    //        PhpShowErrorService(WindowHandle, L"continue", Services[i], status, 0);
    //    }
    //}
    //
    //return result;
}

BOOLEAN PhUiContinueService(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM Service
    )
{
    SC_HANDLE serviceHandle;
    NTSTATUS status;
    BOOLEAN success = FALSE;

    status = PhOpenService(&serviceHandle, SERVICE_PAUSE_CONTINUE, PhGetString(Service->Name));

    if (NT_SUCCESS(status))
    {
        status = PhContinueService(serviceHandle);

        if (NT_SUCCESS(status))
            success = TRUE;

        PhCloseServiceHandle(serviceHandle);
    }

    if (!success)
    {
        BOOLEAN connected;

        if (PhpShowErrorAndConnectToPhSvc(
            WindowHandle,
            PhaConcatStrings2(L"Unable to continue ", PhGetString(Service->Name))->Buffer,
            status,
            &connected
            ))
        {
            if (connected)
            {
                if (NT_SUCCESS(status = PhSvcCallControlService(PhGetString(Service->Name), PhSvcControlServiceContinue)))
                    success = TRUE;
                else
                    PhpShowErrorService(WindowHandle, L"continue", Service, status, 0);

                PhUiDisconnectFromPhSvc();
            }
        }
        else
        {
            PhpShowErrorService(WindowHandle, L"continue", Service, status, 0);
        }
    }

    return success;
}

static NTSTATUS PhUiServicePauseCallback(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    NTSTATUS status;
    SC_HANDLE serviceHandle;

    status = PhOpenService(
        &serviceHandle,
        SERVICE_PAUSE_CONTINUE,
        PhGetString(ServiceItem->Name)
        );

    if (NT_SUCCESS(status))
    {
        status = PhPauseService(serviceHandle);

        PhCloseServiceHandle(serviceHandle);
    }

    return status;
}

BOOLEAN PhUiPauseServices(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    if (PhGetIntegerSetting(L"EnableServiceProgressDialog"))
    {
        PhShowServiceProgressDialog(
            WindowHandle,
            L"pause",
            L"Pausing a service might prevent the system from functioning properly.",
            FALSE,
            Services,
            NumberOfServices,
            PhUiServicePauseCallback,
            PhSvcControlServicePause
            );
        return FALSE;
    }

    if (!PhpShowContinueMessageServices(
        WindowHandle,
        L"pause",
        L"Pausing a service might prevent the system from functioning properly.",
        FALSE,
        Services,
        NumberOfServices
        ))
        return FALSE;

    for (i = 0; i < NumberOfServices; i++)
    {
        NTSTATUS status;
        SC_HANDLE serviceHandle;

        success = FALSE;
        status = PhOpenService(&serviceHandle, SERVICE_PAUSE_CONTINUE, PhGetString(Services[i]->Name));

        if (NT_SUCCESS(status))
        {
            status = PhPauseService(serviceHandle);

            if (NT_SUCCESS(status))
                success = TRUE;

            PhCloseServiceHandle(serviceHandle);
        }

        if (!success)
        {
            BOOLEAN connected;

            success = FALSE;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaConcatStrings2(L"Unable to pause ", PhGetString(Services[i]->Name))->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlService(PhGetString(Services[i]->Name), PhSvcControlServicePause)))
                        success = TRUE;
                    else
                        PhpShowErrorService(WindowHandle, L"pause", Services[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorService(WindowHandle, L"pause", Services[i], status, 0))
                    break;
            }
        }
    }

    return success;

    //BOOLEAN result = TRUE;
    //ULONG i;
    //
    //for (i = 0; i < NumberOfServices; i++)
    //{
    //    SC_HANDLE serviceHandle;
    //    BOOLEAN success = FALSE;
    //
    //    serviceHandle = PhOpenService(PhGetString(Services[i]->Name), SERVICE_PAUSE_CONTINUE);
    //
    //    if (serviceHandle)
    //    {
    //        SERVICE_STATUS serviceStatus;
    //
    //        if (ControlService(serviceHandle, SERVICE_CONTROL_PAUSE, &serviceStatus))
    //            success = TRUE;
    //
    //        PhCloseServiceHandle(serviceHandle);
    //    }
    //
    //    if (!success)
    //    {
    //        NTSTATUS status;
    //
    //        status = PhGetLastWin32ErrorAsNtStatus();
    //        result = FALSE;
    //
    //        PhpShowErrorService(WindowHandle, L"pause", Services[i], status, 0);
    //    }
    //}
    //
    //return result;
}

BOOLEAN PhUiPauseService(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM Service
    )
{
    SC_HANDLE serviceHandle;
    NTSTATUS status;
    BOOLEAN success = FALSE;

    status = PhOpenService(&serviceHandle, SERVICE_PAUSE_CONTINUE, PhGetString(Service->Name));

    if (NT_SUCCESS(status))
    {
        status = PhPauseService(serviceHandle);

        if (NT_SUCCESS(status))
            success = TRUE;

        PhCloseServiceHandle(serviceHandle);
    }

    if (!success)
    {
        BOOLEAN connected;

        if (PhpShowErrorAndConnectToPhSvc(
            WindowHandle,
            PhaConcatStrings2(L"Unable to pause ", Service->Name->Buffer)->Buffer,
            status,
            &connected
            ))
        {
            if (connected)
            {
                if (NT_SUCCESS(status = PhSvcCallControlService(PhGetString(Service->Name), PhSvcControlServicePause)))
                    success = TRUE;
                else
                    PhpShowErrorService(WindowHandle, L"pause", Service, status, 0);

                PhUiDisconnectFromPhSvc();
            }
        }
        else
        {
            PhpShowErrorService(WindowHandle, L"pause", Service, status, 0);
        }
    }

    return success;
}

static NTSTATUS PhUiServiceStopCallback(
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    NTSTATUS status;
    SC_HANDLE serviceHandle;

    status = PhOpenService(
        &serviceHandle,
        SERVICE_STOP,
        PhGetString(ServiceItem->Name)
        );

    if (NT_SUCCESS(status))
    {
        status = PhStopService(serviceHandle);

        PhCloseServiceHandle(serviceHandle);
    }

    return status;
}

BOOLEAN PhUiStopServices(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM* Services,
    _In_ ULONG NumberOfServices
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    if (PhGetIntegerSetting(L"EnableServiceProgressDialog"))
    {
        PhShowServiceProgressDialog(
            WindowHandle,
            L"stop",
            L"Stopping a service might prevent the system from functioning properly.",
            FALSE,
            Services,
            NumberOfServices,
            PhUiServiceStopCallback,
            PhSvcControlServiceStop
            );
        return FALSE;
    }

    if (!PhpShowContinueMessageServices(
        WindowHandle,
        L"stop",
        L"Stopping a service might prevent the system from functioning properly.",
        FALSE,
        Services,
        NumberOfServices
        ))
        return FALSE;

    for (i = 0; i < NumberOfServices; i++)
    {
        NTSTATUS status;
        SC_HANDLE serviceHandle;

        success = FALSE;
        status = PhOpenService(&serviceHandle, SERVICE_STOP, PhGetString(Services[i]->Name));

        if (NT_SUCCESS(status))
        {
            status = PhStopService(serviceHandle);

            if (NT_SUCCESS(status))
                success = TRUE;

            PhCloseServiceHandle(serviceHandle);
        }

        if (!success)
        {
            BOOLEAN connected;

            success = FALSE;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaConcatStrings2(L"Unable to stop ", PhGetString(Services[i]->Name))->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlService(PhGetString(Services[i]->Name), PhSvcControlServiceStop)))
                        success = TRUE;
                    else
                        PhpShowErrorService(WindowHandle, L"stop", Services[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorService(WindowHandle, L"stop", Services[i], status, 0))
                    break;
            }
        }
    }

    return success;

    //BOOLEAN result = TRUE;
    //ULONG i;
    //
    //for (i = 0; i < NumberOfServices; i++)
    //{
    //    SC_HANDLE serviceHandle;
    //    BOOLEAN success = FALSE;
    //
    //    serviceHandle = PhOpenService(PhGetString(Services[i]->Name), SERVICE_STOP);
    //
    //    if (serviceHandle)
    //    {
    //        SERVICE_STATUS serviceStatus;
    //
    //        if (ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
    //            success = TRUE;
    //
    //        PhCloseServiceHandle(serviceHandle);
    //    }
    //
    //    if (!success)
    //    {
    //        NTSTATUS status;
    //
    //        status = PhGetLastWin32ErrorAsNtStatus();
    //        result = FALSE;
    //
    //        PhpShowErrorService(WindowHandle, L"stop", Services[i], status, 0);
    //    }
    //}
    //
    //return result;
}

BOOLEAN PhUiStopService(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM Service
    )
{
    SC_HANDLE serviceHandle;
    NTSTATUS status;
    BOOLEAN success = FALSE;

    status = PhOpenService(&serviceHandle, SERVICE_STOP, PhGetString(Service->Name));

    if (NT_SUCCESS(status))
    {
        status = PhStopService(serviceHandle);

        if (NT_SUCCESS(status))
            success = TRUE;

        PhCloseServiceHandle(serviceHandle);
    }

    if (!success)
    {
        BOOLEAN connected;

        if (PhpShowErrorAndConnectToPhSvc(
            WindowHandle,
            PhaConcatStrings2(L"Unable to stop ", PhGetString(Service->Name))->Buffer,
            status,
            &connected
            ))
        {
            if (connected)
            {
                if (NT_SUCCESS(status = PhSvcCallControlService(PhGetString(Service->Name), PhSvcControlServiceStop)))
                    success = TRUE;
                else
                    PhpShowErrorService(WindowHandle, L"stop", Service, status, 0);

                PhUiDisconnectFromPhSvc();
            }
        }
        else
        {
            PhpShowErrorService(WindowHandle, L"stop", Service, status, 0);
        }
    }

    return success;
}

BOOLEAN PhUiDeleteService(
    _In_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM Service
    )
{
    SC_HANDLE serviceHandle;
    NTSTATUS status;
    BOOLEAN success = FALSE;

    // Warnings cannot be disabled for service deletion.
    if (!PhShowConfirmMessage(
        WindowHandle,
        L"delete",
        Service->Name->Buffer,
        L"Deleting a service can prevent the system from starting "
        L"or functioning properly.",
        TRUE
        ))
        return FALSE;

    status = PhOpenService(&serviceHandle, DELETE, PhGetString(Service->Name));

    if (NT_SUCCESS(status))
    {
        status = PhDeleteService(serviceHandle);

        if (NT_SUCCESS(status))
            success = TRUE;

        PhCloseServiceHandle(serviceHandle);
    }

    if (!success)
    {
        BOOLEAN connected;

        if (PhpShowErrorAndConnectToPhSvc(
            WindowHandle,
            PhaConcatStrings2(L"Unable to delete ", PhGetString(Service->Name))->Buffer,
            status,
            &connected
            ))
        {
            if (connected)
            {
                if (NT_SUCCESS(status = PhSvcCallControlService(PhGetString(Service->Name), PhSvcControlServiceDelete)))
                    success = TRUE;
                else
                    PhpShowErrorService(WindowHandle, L"delete", Service, status, 0);

                PhUiDisconnectFromPhSvc();
            }
        }
        else
        {
            PhpShowErrorService(WindowHandle, L"delete", Service, status, 0);
        }
    }

    return success;
}

BOOLEAN PhUiCloseConnections(
    _In_ HWND WindowHandle,
    _In_ PPH_NETWORK_ITEM *Connections,
    _In_ ULONG NumberOfConnections
    )
{
    ULONG (WINAPI* SetTcpEntry_I)(_In_ PMIB_TCPROW pTcpRow) = NULL;
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG result;
    ULONG i;
    MIB_TCPROW tcpRow;

    SetTcpEntry_I = PhGetDllProcedureAddress(L"iphlpapi.dll", "SetTcpEntry", 0);

    if (!SetTcpEntry_I)
    {
        PhShowError(
            WindowHandle,
            L"%s",
            L"This feature is not supported by your operating system."
            );
        return FALSE;
    }

    for (i = 0; i < NumberOfConnections; i++)
    {
        if (
            Connections[i]->ProtocolType != PH_TCP4_NETWORK_PROTOCOL ||
            Connections[i]->State != MIB_TCP_STATE_ESTAB
            )
            continue;

        tcpRow.dwState = MIB_TCP_STATE_DELETE_TCB;
        tcpRow.dwLocalAddr = Connections[i]->LocalEndpoint.Address.Ipv4;
        tcpRow.dwLocalPort = _byteswap_ushort((USHORT)Connections[i]->LocalEndpoint.Port);
        tcpRow.dwRemoteAddr = Connections[i]->RemoteEndpoint.Address.Ipv4;
        tcpRow.dwRemotePort = _byteswap_ushort((USHORT)Connections[i]->RemoteEndpoint.Port);

        if ((result = SetTcpEntry_I(&tcpRow)) != NO_ERROR)
        {
            NTSTATUS status;
            BOOLEAN connected;

            success = FALSE;

            // SetTcpEntry returns ERROR_MR_MID_NOT_FOUND for access denied errors for some reason.
            if (result == ERROR_MR_MID_NOT_FOUND)
                result = ERROR_ACCESS_DENIED;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                L"Unable to close the TCP connection",
                PhDosErrorToNtStatus(result),
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallSetTcpEntry(&tcpRow)))
                        success = TRUE;
                    else
                        PhShowStatus(WindowHandle, L"Unable to close the TCP connection", status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (PhShowMessage2(
                    WindowHandle,
                    TD_OK_BUTTON,
                    TD_ERROR_ICON,
                    L"Unable to close the TCP connection.",
                    L"Make sure System Informer is running with administrative privileges."
                    ) != IDOK)
                    break;
            }
        }
    }

    return success;
}

static BOOLEAN PhpShowContinueMessageThreads(
    _In_ HWND WindowHandle,
    _In_ PWSTR Verb,
    _In_ PWSTR Message,
    _In_ BOOLEAN Warning,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    )
{
    PWSTR object;
    BOOLEAN cont = FALSE;

    if (NumberOfThreads == 0)
        return FALSE;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        if (NumberOfThreads == 1)
        {
            object = L"the selected thread";
        }
        else
        {
            object = L"the selected threads";
        }

        cont = PhShowConfirmMessage(
            WindowHandle,
            Verb,
            object,
            Message,
            Warning
            );
    }
    else
    {
        cont = TRUE;
    }

    return cont;
}

static BOOLEAN PhpShowErrorThread(
    _In_ HWND WindowHandle,
    _In_ PWSTR Verb,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    return PhShowContinueStatus(
        WindowHandle,
        PhaFormatString(
        L"Unable to %s thread %lu",
        Verb,
        HandleToUlong(Thread->ThreadId)
        )->Buffer,
        Status,
        Win32Result
        );
}

BOOLEAN PhUiTerminateThreads(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    if (!PhpShowContinueMessageThreads(
        WindowHandle,
        L"terminate",
        L"Terminating a thread may cause the process to stop working.",
        FALSE,
        Threads,
        NumberOfThreads
        ))
        return FALSE;

    for (i = 0; i < NumberOfThreads; i++)
    {
        NTSTATUS status;
        HANDLE threadHandle;

        if (NT_SUCCESS(status = PhOpenThread(
            &threadHandle,
            THREAD_TERMINATE,
            Threads[i]->ThreadId
            )))
        {
            status = PhTerminateThread(threadHandle, STATUS_SUCCESS);

            if (status == STATUS_SUCCESS || status == STATUS_THREAD_IS_TERMINATING)
                PhTerminateThread(threadHandle, DBG_TERMINATE_THREAD); // debug terminate (dmex)

            NtClose(threadHandle);
        }

        if (!NT_SUCCESS(status))
        {
            BOOLEAN connected;

            success = FALSE;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaFormatString(L"Unable to terminate thread %lu", HandleToUlong(Threads[i]->ThreadId))->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlThread(Threads[i]->ThreadId, PhSvcControlThreadTerminate, 0)))
                        success = TRUE;
                    else
                        PhpShowErrorThread(WindowHandle, L"terminate", Threads[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorThread(WindowHandle, L"terminate", Threads[i], status, 0))
                    break;
            }
        }
    }

    return success;
}

BOOLEAN PhUiSuspendThreads(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    for (i = 0; i < NumberOfThreads; i++)
    {
        NTSTATUS status;
        HANDLE threadHandle;

        if (NT_SUCCESS(status = PhOpenThread(
            &threadHandle,
            THREAD_SUSPEND_RESUME,
            Threads[i]->ThreadId
            )))
        {
            status = NtSuspendThread(threadHandle, NULL);
            NtClose(threadHandle);
        }

        if (!NT_SUCCESS(status))
        {
            BOOLEAN connected;

            success = FALSE;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaFormatString(L"Unable to suspend thread %lu", HandleToUlong(Threads[i]->ThreadId))->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlThread(Threads[i]->ThreadId, PhSvcControlThreadSuspend, 0)))
                        success = TRUE;
                    else
                        PhpShowErrorThread(WindowHandle, L"suspend", Threads[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorThread(WindowHandle, L"suspend", Threads[i], status, 0))
                    break;
            }
        }
    }

    return success;
}

BOOLEAN PhUiResumeThreads(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    )
{
    BOOLEAN success = TRUE;
    BOOLEAN cancelled = FALSE;
    ULONG i;

    for (i = 0; i < NumberOfThreads; i++)
    {
        NTSTATUS status;
        HANDLE threadHandle;

        if (NT_SUCCESS(status = PhOpenThread(
            &threadHandle,
            THREAD_SUSPEND_RESUME,
            Threads[i]->ThreadId
            )))
        {
            status = NtResumeThread(threadHandle, NULL);
            NtClose(threadHandle);
        }

        if (!NT_SUCCESS(status))
        {
            BOOLEAN connected;

            success = FALSE;

            if (!cancelled && PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaFormatString(L"Unable to resume thread %lu", HandleToUlong(Threads[i]->ThreadId))->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallControlThread(Threads[i]->ThreadId, PhSvcControlThreadResume, 0)))
                        success = TRUE;
                    else
                        PhpShowErrorThread(WindowHandle, L"resume", Threads[i], status, 0);

                    PhUiDisconnectFromPhSvc();
                }
                else
                {
                    cancelled = TRUE;
                }
            }
            else
            {
                if (!PhpShowErrorThread(WindowHandle, L"resume", Threads[i], status, 0))
                    break;
            }
        }
    }

    return success;
}

BOOLEAN PhUiSetBoostPriorityThreads(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads,
    _In_ BOOLEAN PriorityBoost
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    for (i = 0; i < NumberOfThreads; i++)
    {
        NTSTATUS status;
        HANDLE threadHandle;

        status = PhOpenThread(
            &threadHandle,
            THREAD_SET_LIMITED_INFORMATION,
            Threads[i]->ThreadId
            );

        if (NT_SUCCESS(status))
        {
            status = PhSetThreadPriorityBoost(threadHandle, PriorityBoost);
            NtClose(threadHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorThread(WindowHandle, L"change boost priority of", Threads[i], status, 0))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiSetBoostPriorityThread(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ BOOLEAN PriorityBoost
    )
{
    NTSTATUS status;
    HANDLE threadHandle;

    if (NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_SET_LIMITED_INFORMATION,
        Thread->ThreadId
        )))
    {
        status = PhSetThreadPriorityBoost(threadHandle, PriorityBoost);
        NtClose(threadHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorThread(WindowHandle, L"set the boost priority of", Thread, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetPriorityThreads(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads,
    _In_ LONG Increment
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    // Special saturation values
    if (Increment == THREAD_PRIORITY_TIME_CRITICAL)
        Increment = THREAD_BASE_PRIORITY_LOWRT + 1;
    else if (Increment == THREAD_PRIORITY_IDLE)
        Increment = THREAD_BASE_PRIORITY_IDLE - 1;

    for (i = 0; i < NumberOfThreads; i++)
    {
        NTSTATUS status;
        HANDLE threadHandle;

        status = PhOpenThread(
            &threadHandle,
            THREAD_SET_LIMITED_INFORMATION,
            Threads[i]->ThreadId
            );

        if (NT_SUCCESS(status))
        {
            status = PhSetThreadBasePriority(threadHandle, Increment);
            NtClose(threadHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorThread(WindowHandle, L"change priority of", Threads[i], status, 0))
                break;
        }
    }

    return success;
}

BOOLEAN PhUiSetPriorityThread(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ LONG Increment
    )
{
    NTSTATUS status;
    HANDLE threadHandle;

    if (NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_SET_LIMITED_INFORMATION,
        Thread->ThreadId
        )))
    {
        status = PhSetThreadBasePriority(threadHandle, Increment);
        NtClose(threadHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorThread(WindowHandle, L"set the priority of", Thread, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiSetIoPriorityThread(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ IO_PRIORITY_HINT IoPriority
    )
{
    NTSTATUS status;
    BOOLEAN success = TRUE;
    HANDLE threadHandle;

    if (NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_SET_INFORMATION,
        Thread->ThreadId
        )))
    {
        status = PhSetThreadIoPriority(threadHandle, IoPriority);
        NtClose(threadHandle);
    }

    if (!NT_SUCCESS(status))
    {
        BOOLEAN connected;

        success = FALSE;

        // The operation may have failed due to the lack of SeIncreaseBasePriorityPrivilege.
        if (PhpShowErrorAndConnectToPhSvc(
            WindowHandle,
            PhaFormatString(L"Unable to set the I/O priority of thread %lu", HandleToUlong(Thread->ThreadId))->Buffer,
            status,
            &connected
            ))
        {
            if (connected)
            {
                if (NT_SUCCESS(status = PhSvcCallControlThread(Thread->ThreadId, PhSvcControlThreadIoPriority, IoPriority)))
                    success = TRUE;
                else
                    PhpShowErrorThread(WindowHandle, L"set the I/O priority of", Thread, status, 0);

                PhUiDisconnectFromPhSvc();
            }
        }
        else
        {
            PhpShowErrorThread(WindowHandle, L"set the I/O priority of", Thread, status, 0);
        }
    }

    return success;
}

BOOLEAN PhUiSetPagePriorityThread(
    _In_ HWND WindowHandle,
    _In_ PPH_THREAD_ITEM Thread,
    _In_ ULONG PagePriority
    )
{
    NTSTATUS status;
    HANDLE threadHandle;

    if (NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_SET_INFORMATION,
        Thread->ThreadId
        )))
    {
        status = PhSetThreadPagePriority(threadHandle, PagePriority);

        NtClose(threadHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorThread(WindowHandle, L"set the page priority of", Thread, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiUnloadModule(
    _In_ HWND WindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_MODULE_ITEM Module
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE processHandle = NULL;
    BOOLEAN cont = FALSE;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        PWSTR verb;
        PWSTR message;

        switch (Module->Type)
        {
        case PH_MODULE_TYPE_MODULE:
        case PH_MODULE_TYPE_WOW64_MODULE:
            verb = L"unload";
            message = L"Unloading a module may cause the process to crash.";

            if (WindowsVersion >= WINDOWS_8)
                message = L"Unloading a module may cause the process to crash. NOTE: This feature may not work correctly on your version of Windows and some programs may restrict access or ban your account.";

            break;
        case PH_MODULE_TYPE_KERNEL_MODULE:
            verb = L"unload";
            message = L"Unloading a driver may cause system instability.";
            break;
        case PH_MODULE_TYPE_MAPPED_FILE:
        case PH_MODULE_TYPE_MAPPED_IMAGE:
            verb = L"unmap";
            message = L"Unmapping a section view may cause the process to crash.";
            break;
        default:
            return FALSE;
        }

        cont = PhShowConfirmMessage(
            WindowHandle,
            verb,
            Module->Name->Buffer,
            message,
            TRUE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    switch (Module->Type)
    {
    case PH_MODULE_TYPE_MODULE:
    case PH_MODULE_TYPE_WOW64_MODULE:
        {
            if (WindowsVersion < WINDOWS_8)
            {
                // Windows 7 requires QUERY_INFORMATION for MemoryMappedFileName. (dmex)

                status = PhOpenProcess(
                    &processHandle,
                    PROCESS_QUERY_INFORMATION | PROCESS_SET_LIMITED_INFORMATION |
                    PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
                    PROCESS_VM_READ | PROCESS_VM_WRITE,
                    ProcessId
                    );
            }
            else if (WindowsVersion < WINDOWS_10)
            {
                // Windows 8 requires ALL_ACCESS for PLM execution requests. (dmex)

                status = PhOpenProcess(
                    &processHandle,
                    PROCESS_ALL_ACCESS,
                    ProcessId
                    );
            }

            if (!NT_SUCCESS(status))
            {
                // Windows 10 and above require SET_LIMITED for PLM execution requests. (dmex)

                status = PhOpenProcess(
                    &processHandle,
                    PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_LIMITED_INFORMATION |
                    PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION |
                    PROCESS_VM_READ | PROCESS_VM_WRITE,
                    ProcessId
                    );
            }

            if (NT_SUCCESS(status))
            {
                LARGE_INTEGER timeout;

                timeout.QuadPart = -(LONGLONG)UInt32x32To64(5, PH_TIMEOUT_SEC);
                status = PhUnloadDllProcess(
                    processHandle,
                    Module->BaseAddress,
                    &timeout
                    );

                NtClose(processHandle);
            }

            if (status == STATUS_DLL_NOT_FOUND)
            {
                PhShowError(WindowHandle, L"%s", L"Unable to find the module to unload.");
                return FALSE;
            }

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(
                    WindowHandle,
                    PhaConcatStrings2(L"Unable to unload ", Module->Name->Buffer)->Buffer,
                    status,
                    0
                    );
                return FALSE;
            }
        }
        break;

    case PH_MODULE_TYPE_KERNEL_MODULE:
        status = PhUnloadDriver(Module->BaseAddress, Module->Name->Buffer);

        if (!NT_SUCCESS(status))
        {
            BOOLEAN success = FALSE;
            BOOLEAN connected;

            if (PhpShowErrorAndConnectToPhSvc(
                WindowHandle,
                PhaConcatStrings2(L"Unable to unload ", Module->Name->Buffer)->Buffer,
                status,
                &connected
                ))
            {
                if (connected)
                {
                    if (NT_SUCCESS(status = PhSvcCallUnloadDriver(Module->BaseAddress, Module->Name->Buffer)))
                        success = TRUE;
                    else
                        PhShowStatus(WindowHandle, PhaConcatStrings2(L"Unable to unload ", Module->Name->Buffer)->Buffer, status, 0);

                    PhUiDisconnectFromPhSvc();
                }
            }
            else
            {
                PhShowStatus(
                    WindowHandle,
                    PhaConcatStrings(
                    3,
                    L"Unable to unload ",
                    Module->Name->Buffer,
                    L". Make sure System Informer is running with "
                    L"administrative privileges."
                    )->Buffer,
                    status,
                    0
                    );
                return FALSE;
            }

            return success;
        }

        break;

    case PH_MODULE_TYPE_MAPPED_FILE:
    case PH_MODULE_TYPE_MAPPED_IMAGE:
        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_VM_OPERATION,
            ProcessId
            )))
        {
            status = NtUnmapViewOfSection(processHandle, Module->BaseAddress);
            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            PhShowStatus(
                WindowHandle,
                PhaFormatString(L"Unable to unmap the section view at 0x%p", Module->BaseAddress)->Buffer,
                status,
                0
                );
            return FALSE;
        }

        break;

    default:
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiFreeMemory(
    _In_ HWND WindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_MEMORY_ITEM MemoryItem,
    _In_ BOOLEAN Free
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    HANDLE processHandle;

    if (PhGetIntegerSetting(L"EnableWarnings"))
    {
        PWSTR verb;
        PWSTR message;

        if (!(MemoryItem->Type & (MEM_MAPPED | MEM_IMAGE)))
        {
            if (Free)
            {
                verb = L"free";
                message = L"Freeing memory regions may cause the process to crash.\r\n\r\nSome programs may also restrict access or ban your account when freeing the memory of the process.";
            }
            else
            {
                verb = L"decommit";
                message = L"Decommitting memory regions may cause the process to crash.\r\n\r\nSome programs may also restrict access or ban your account when decommitting the memory of the process.";
            }
        }
        else
        {
            verb = L"unmap";
            message = L"Unmapping a section view may cause the process to crash.\r\n\r\nSome programs may also restrict access or ban your account when unmapping the memory of the process.";
        }

        cont = PhShowConfirmMessage(
            WindowHandle,
            verb,
            L"the memory region",
            message,
            TRUE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_VM_OPERATION,
        ProcessId
        )))
    {
        PVOID baseAddress;
        SIZE_T regionSize;

        baseAddress = MemoryItem->BaseAddress;

        if (!(MemoryItem->Type & (MEM_MAPPED | MEM_IMAGE)))
        {
            // The size needs to be 0 if we're freeing.
            if (Free)
                regionSize = 0;
            else
                regionSize = MemoryItem->RegionSize;

            status = NtFreeVirtualMemory(
                processHandle,
                &baseAddress,
                &regionSize,
                Free ? MEM_RELEASE : MEM_DECOMMIT
                );
        }
        else
        {
            status = NtUnmapViewOfSection(processHandle, baseAddress);
        }

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PWSTR message;

        if (!(MemoryItem->Type & (MEM_MAPPED | MEM_IMAGE)))
        {
            if (Free)
                message = L"Unable to free the memory region";
            else
                message = L"Unable to decommit the memory region";
        }
        else
        {
            message = L"Unable to unmap the section view";
        }

        PhShowStatus(
            WindowHandle,
            message,
            status,
            0
            );
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN PhpShowErrorHandle(
    _In_ HWND WindowHandle,
    _In_ PWSTR Verb,
    _In_ PPH_HANDLE_ITEM Handle,
    _In_ NTSTATUS Status,
    _In_opt_ ULONG Win32Result
    )
{
    WCHAR value[PH_PTR_STR_LEN_1];

    PhPrintPointer(value, (PVOID)Handle->Handle);

    if (!PhIsNullOrEmptyString(Handle->BestObjectName))
    {
        return PhShowContinueStatus(
            WindowHandle,
            PhaFormatString(
            L"Unable to %s handle \"%s\" (%s)",
            Verb,
            Handle->BestObjectName->Buffer,
            value
            )->Buffer,
            Status,
            Win32Result
            );
    }
    else
    {
        return PhShowContinueStatus(
            WindowHandle,
            PhaFormatString(
            L"Unable to %s handle %s",
            Verb,
            value
            )->Buffer,
            Status,
            Win32Result
            );
    }
}

BOOLEAN PhUiCloseHandles(
    _In_ HWND WindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM *Handles,
    _In_ ULONG NumberOfHandles,
    _In_ BOOLEAN Warn
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    BOOLEAN success = TRUE;
    HANDLE processHandle;

    if (NumberOfHandles == 0)
        return FALSE;

    if (Warn && PhGetIntegerSetting(L"EnableWarnings"))
    {
        cont = PhShowConfirmMessage(
            WindowHandle,
            L"close",
            NumberOfHandles == 1 ? L"the selected handle" : L"the selected handles",
            L"Closing handles may cause system instability and data corruption.",
            FALSE
            );
    }
    else
    {
        cont = TRUE;
    }

    if (!cont)
        return FALSE;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE,
        ProcessId
        )))
    {
        BOOLEAN critical = FALSE;
        BOOLEAN strict = FALSE;

        if (WindowsVersion >= WINDOWS_10)
        {
            BOOLEAN breakOnTermination;
            PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

            if (NT_SUCCESS(PhGetProcessBreakOnTermination(
                processHandle,
                &breakOnTermination
                )))
            {
                if (breakOnTermination)
                {
                    critical = TRUE;
                }
            }

            if (NT_SUCCESS(PhGetProcessMitigationPolicyInformation(
                processHandle,
                ProcessStrictHandleCheckPolicy,
                &policyInfo
                )))
            {
                if (policyInfo.StrictHandleCheckPolicy.Flags != 0)
                {
                    strict = TRUE;
                }
            }
        }

        if (critical && strict)
        {
            cont = PhShowConfirmMessage(
                WindowHandle,
                L"close",
                L"critical process handle(s)",
                L"You are about to close one or more handles for a critical process with strict handle checks enabled. This will shut down the operating system immediately.\r\n\r\n",
                TRUE
                );
        }

        if (!cont)
            return FALSE;

        for (ULONG i = 0; i < NumberOfHandles; i++)
        {
            status = NtDuplicateObject(
                processHandle,
                Handles[i]->Handle,
                NULL,
                NULL,
                0,
                0,
                DUPLICATE_CLOSE_SOURCE
                );

            if (!NT_SUCCESS(status))
            {
                success = FALSE;

                if (!PhpShowErrorHandle(
                    WindowHandle,
                    L"close",
                    Handles[i],
                    status,
                    0
                    ))
                    break;
            }
        }

        NtClose(processHandle);
    }
    else
    {
        PhShowStatus(WindowHandle, L"Unable to open the process", status, 0);
        return FALSE;
    }

    return success;
}

BOOLEAN PhUiSetAttributesHandle(
    _In_ HWND WindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM Handle,
    _In_ ULONG Attributes
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (KsiLevel() < KphLevelMax)
    {
        PhShowKsiNotConnected(
            WindowHandle,
            L"Setting handle attributes requires a connection to the kernel driver."
            );
        return FALSE;
    }

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION,
        ProcessId
        )))
    {
        OBJECT_HANDLE_FLAG_INFORMATION handleFlagInfo;

        handleFlagInfo.Inherit = !!(Attributes & OBJ_INHERIT);
        handleFlagInfo.ProtectFromClose = !!(Attributes & OBJ_PROTECT_CLOSE);

        status = KphSetInformationObject(
            processHandle,
            Handle->Handle,
            KphObjectHandleFlagInformation,
            &handleFlagInfo,
            sizeof(OBJECT_HANDLE_FLAG_INFORMATION)
            );

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
    {
        PhpShowErrorHandle(WindowHandle, L"set attributes of", Handle, status, 0);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN PhUiFlushHeapProcesses(
    _In_ HWND WindowHandle,
    _In_ PPH_PROCESS_ITEM *Processes,
    _In_ ULONG NumberOfProcesses
    )
{
    BOOLEAN success = TRUE;
    ULONG i;

    for (i = 0; i < NumberOfProcesses; i++)
    {
        NTSTATUS status;
        HANDLE processHandle;

        status = PhOpenProcess(
            &processHandle,
            PROCESS_CREATE_THREAD | PROCESS_QUERY_LIMITED_INFORMATION |
            PROCESS_SET_LIMITED_INFORMATION | PROCESS_VM_READ,
            Processes[i]->ProcessId
            );

        if (NT_SUCCESS(status))
        {
            status = PhFlushProcessHeapsRemote(processHandle, PhTimeoutFromMillisecondsEx(4000));
            NtClose(processHandle);
        }

        if (!NT_SUCCESS(status))
        {
            success = FALSE;

            if (!PhpShowErrorProcess(WindowHandle, L"flush the process heap(s) of", Processes[i], status, 0))
                break;
        }
    }

    return success;
}

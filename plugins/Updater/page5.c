/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2022
 *
 */

#include "updater.h"

static TASKDIALOG_BUTTON TaskDialogButtonArray[] =
{
    { IDYES, L"Install" }
};

BOOLEAN UpdaterCheckApplicationDirectory(
    VOID
    )
{
    static PH_STRINGREF fileNameStringRef = PH_STRINGREF_INIT(L"\\test");
    HANDLE fileHandle;
    PPH_STRING fileName;

    fileName = PhGetApplicationDirectoryFileName(&fileNameStringRef, TRUE);

    if (PhIsNullOrEmptyString(fileName))
        return FALSE;

    if (NT_SUCCESS(PhCreateFile(
        &fileHandle,
        &fileName->sr,
        FILE_GENERIC_WRITE | DELETE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE
        )))
    {
        PhDereferenceObject(fileName);
        NtClose(fileHandle);
        return TRUE;
    }

    PhDereferenceObject(fileName);
    return FALSE;
}

HRESULT CALLBACK FinalTaskDialogCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        {
            if (!UpdaterCheckApplicationDirectory())
            {
                SendMessage(hwndDlg, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDYES, TRUE);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            INT buttonId = (INT)wParam;

            if (buttonId == IDRETRY)
            {
                ShowCheckForUpdatesDialog(context);
                return S_FALSE;
            }
            else if (buttonId == IDYES)
            {
                PVOID parameters;

                if (PhIsNullOrEmptyString(context->SetupFilePath))
                    break;

                parameters = PH_AUTO(PhCreateKsiSettingsBlob());
                parameters = PH_AUTO(PhConcatStrings(3, L"-update \"", PhGetStringOrEmpty(parameters), L"\""));

                ProcessHacker_PrepareForEarlyShutdown();

                if (PhShellExecuteEx(
                    hwndDlg,
                    PhGetString(context->SetupFilePath),
                    PhGetString(parameters),
                    SW_SHOW,
                    PH_SHELL_EXECUTE_NOZONECHECKS | PH_SHELL_EXECUTE_NOASYNC | (UpdaterCheckApplicationDirectory() ? 0 : PH_SHELL_EXECUTE_ADMIN),
                    0,
                    NULL
                    ))
                {
                    ProcessHacker_Destroy();
                }
                else
                {
                    ULONG errorCode = GetLastError();

                    // Install failed, cancel the shutdown.
                    ProcessHacker_CancelEarlyShutdown();

                    // Show error dialog.
                    if (errorCode != ERROR_CANCELLED) // Ignore UAC decline.
                    {
                        PhShowStatus(hwndDlg, L"Unable to execute the setup.", 0, errorCode);

                        if (context->StartupCheck)
                            ShowAvailableDialog(context);
                        else
                            ShowCheckForUpdatesDialog(context);
                    }

                    return S_FALSE;
                }
            }
        }
        break;
    case TDN_HYPERLINK_CLICKED:
        {
            TaskDialogLinkClicked(context);
            return S_FALSE;
        }
        break;
    }

    return S_OK;
}

VOID ShowUpdateInstallDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = RTL_NUMBER_OF(TaskDialogButtonArray);

    config.pszWindowTitle = L"System Informer - Updater";
    config.pszMainInstruction = L"Ready to install update?";
    config.pszContent = L"The update has been successfully downloaded and verified.\r\n\r\nClick Install to continue.";

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

PPH_STRING UpdaterGetLatestVersionText(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PPH_STRING version;
    PPH_STRING commit;
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;
    ULONG revisionVersion;

    PhGetPhVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    commit = PhGetPhVersionHash();

    if (commit && commit->Length > 4)
    {
        version = PhFormatString(
            L"%lu.%lu.%lu (%s)",
            majorVersion,
            minorVersion,
            revisionVersion,
            PhGetString(commit)
            );
        PhMoveReference(&version, PhFormatString(
            L"%s\r\n\r\n<A HREF=\"changelog.txt\">View changelog</A>",
            PhGetStringOrEmpty(version)
            ));
    }
    else
    {
        version = PhFormatString(
            L"System Informer %lu.%lu.%lu",
            majorVersion,
            minorVersion,
            revisionVersion
            );
        PhMoveReference(&version, PhFormatString(
            L"%s\r\n\r\n<A HREF=\"changelog.txt\">View changelog</A>",
            PhGetStringOrEmpty(version)
            ));
    }

    if (commit)
    {
        PhDereferenceObject(commit);
    }

    return version;
}

VOID ShowLatestVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.pszWindowTitle = L"System Informer - Updater";
    config.pszMainInstruction = L"You're running the latest version.";
    config.pszContent = PH_AUTO_T(PH_STRING, UpdaterGetLatestVersionText(Context))->Buffer;

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

VOID ShowNewerVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.pszWindowTitle = L"System Informer - Updater";
    config.pszMainInstruction = L"You're running a pre-release build.";
    config.pszContent = PhaFormatString(
        L"Pre-release build: v%s\r\n",
        PhGetStringOrEmpty(Context->CurrentVersionString)
        )->Buffer;

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

VOID ShowUpdateFailedDialog(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ BOOLEAN HashFailed,
    _In_ BOOLEAN SignatureFailed
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    //config.pszMainIcon = MAKEINTRESOURCE(65529);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON | TDCBF_RETRY_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);

    config.pszWindowTitle = L"System Informer - Updater";
    config.pszMainInstruction = L"Error downloading the update.";

    if (SignatureFailed)
    {
        config.pszContent = L"Signature check failed. Click Retry to download the update again.";
    }
    else if (HashFailed)
    {
        config.pszContent = L"Hash check failed. Click Retry to download the update again.";
    }
    else
    {
        if (Context->ErrorCode)
        {
            PPH_STRING errorMessage;

            if (errorMessage = PhHttpSocketGetErrorMessage(Context->ErrorCode))
            {
                config.pszContent = PhaFormatString(L"[%lu] %s", Context->ErrorCode, errorMessage->Buffer)->Buffer;
                PhDereferenceObject(errorMessage);
            }
            else
            {
                config.pszContent = L"Click Retry to download the update again.";
            }
        }
        else
        {
            config.pszContent = L"Click Retry to download the update again.";
        }
    }

    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    TaskDialogNavigatePage(Context->DialogHandle, &config);
}

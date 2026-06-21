/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2023
 *
 */

#include "updater.h"

/**
 * \brief Callback procedure for the final state task dialog pages (Install, Latest, Error).
 * \param WindowHandle Handle to the dialog window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \param dwRefData The updater context.
 * \return HRESULT Successful or errant status.
 */
HRESULT CALLBACK FinalTaskDialogCallbackProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (WindowMessage)
    {
    case TDN_NAVIGATED:
        {
#ifndef FORCE_NO_STATUS_TIMER
            if (context->ProgressTimer)
            {
                PhKillTimer(WindowHandle, 9000);
                context->ProgressTimer = FALSE;
            }
#endif
            context->ElevationRequired = !!UpdateCheckDirectoryElevationRequired();

#ifdef FORCE_ELEVATION_CHECK
            context->ElevationRequired = TRUE;
#endif

            if (context->ElevationRequired)
            {
                SendMessage(WindowHandle, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDYES, TRUE);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            INT buttonId = (INT)wParam;

            if (buttonId == IDRETRY)
            {
                if (context->CryptoBackend == UpdaterCryptoBackendSymCrypt)
                    context->CryptoBackend = UpdaterCryptoBackendBCrypt;
                ShowCheckForUpdatesDialog(context);
                return S_FALSE;
            }
            else if (buttonId == IDYES)
            {
#if defined(PH_BUILD_MSIX)
                // MSIX: the platform already downloaded and installed the update.
                // Nothing to ShellExecute; let the dialog close.
#else
                if (!NT_SUCCESS(UpdateShellExecute(context, WindowHandle)))
                {
                    return S_FALSE;
                }
#endif
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

/**
 * \brief Shows the Ready to Install dialog page.
 * \param Context The updater context.
 */
VOID ShowUpdateInstallDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOG_BUTTON TaskDialogButtonArray[] =
    {
        { IDYES, L"Install" }
    };
    TASKDIALOGCONFIG config;

    if (PhGetIntegerSetting(SETTING_NAME_TOAST_NOTIFICATIONS))
    {
        if (UpdaterShowReadyToInstallToast(Context))
            return;
    }

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE, Context->WindowDpi);
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;
    config.pButtons = TaskDialogButtonArray;
    config.cButtons = RTL_NUMBER_OF(TaskDialogButtonArray);

    config.pszWindowTitle = L"System Informer - Updater";
    if (Context->SwitchingChannel)
    {
        switch (Context->Channel)
        {
        case PhReleaseChannel:
            config.pszMainInstruction = L"Ready to switch to the release channel?";
            break;
        //case PhPreviewChannel:
        //    config.pszMainInstruction = L"Ready to switch to the preview channel?";
        //    break;
        case PhCanaryChannel:
            config.pszMainInstruction = L"Ready to switch to the canary channel?";
            break;
        //case PhDeveloperChannel:
        //    config.pszMainInstruction = L"Ready to switch to the developer channel?";
        //    break;
        default:
            config.pszMainInstruction = L"Ready to switch the channel?";
            break;
        }

        config.pszContent = L"The channel has been successfully downloaded and verified.\r\n\r\nClick Install to continue.";
    }
    else
    {
#if defined(PH_BUILD_MSIX)
        config.pszMainInstruction = L"Update installed.";
        config.pszContent = L"The update has been downloaded and installed.\r\n\r\nRestart System Informer to apply the update.";
#else
        config.pszMainInstruction = L"Ready to install update?";
        config.pszContent = L"The update has been successfully downloaded and verified.\r\n\r\nClick Install to continue.";
#endif
    }

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

/**
 * \brief Generates the text describing the current latest version.
 * \param Context The updater context.
 * \return A string containing the formatted version text.
 */
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

    PhGetBuildVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    commit = PhGetBuildCommit();

    if (commit && commit->Length > 4)
    {
        version = PhFormatString(
            L"%lu.%lu.%lu.%lu (%s)",
            majorVersion,
            minorVersion,
            buildVersion,
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
            L"System Informer %lu.%lu.%lu.%lu",
            majorVersion,
            minorVersion,
            buildVersion,
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

/**
 * \brief Shows the Latest Version dialog page.
 * \param Context The updater context.
 */
VOID ShowLatestVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE, Context->WindowDpi);
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.pszWindowTitle = L"System Informer - Updater";
    config.pszMainInstruction = L"You're running the latest version.";
    config.pszContent = PH_AUTO_T(PH_STRING, UpdaterGetLatestVersionText(Context))->Buffer;

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

/**
 * \brief Shows the Newer Version dialog page (e.g., when running a pre-release).
 * \param Context The updater context.
 */
VOID ShowNewerVersionDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_ENABLE_HYPERLINKS;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE, Context->WindowDpi);
    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.pszWindowTitle = L"System Informer - Updater";
    config.pszMainInstruction = L"You're running a pre-release build.";
    config.pszContent = PH_AUTO_T(PH_STRING, UpdaterGetLatestVersionText(Context))->Buffer;

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

/**
 * \brief Shows the Update Failed dialog page.
 * \param Context The updater context.
 * \param HashFailed TRUE if the hash verification failed.
 * \param SignatureFailed TRUE if the signature verification failed.
 */
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
    config.hMainIcon = PhGetApplicationIcon(FALSE, Context->WindowDpi);

    config.pszWindowTitle = L"System Informer - Updater";
    if (Context->SwitchingChannel)
        config.pszMainInstruction = L"Error downloading the channel.";
    else
        config.pszMainInstruction = L"Error downloading the update.";

    if (SignatureFailed)
    {
        if (Context->SwitchingChannel)
            config.pszContent = L"Signature check failed. Click Retry to download the channel again.";
        else
            config.pszContent = L"Signature check failed. Click Retry to download the update again.";
    }
    else if (HashFailed)
    {
        if (Context->SwitchingChannel)
            config.pszContent = L"Hash check failed. Click Retry to download the channel again.";
        else
            config.pszContent = L"Hash check failed. Click Retry to download the update again.";
    }
    else
    {
        if (Context->UpdateStatus)
        {
            PPH_STRING errorMessage;

            if (errorMessage = PhHttpGetErrorMessage(Context->UpdateStatus))
            {
                config.pszContent = PhaFormatString(L"[%lu] %s", Context->UpdateStatus, errorMessage->Buffer)->Buffer;
                PhDereferenceObject(errorMessage);
            }
            else if (errorMessage = PhGetStatusMessage(Context->UpdateStatus, 0))
            {
                config.pszContent = PhaFormatString(L"[%lu] %s", Context->UpdateStatus, errorMessage->Buffer)->Buffer;
                PhDereferenceObject(errorMessage);
            }
            else
            {
                if (Context->SwitchingChannel)
                    config.pszContent = L"Click Retry to download the channel again.";
                else
                    config.pszContent = L"Click Retry to download the update again.";
            }
        }
        else
        {
            if (Context->SwitchingChannel)
                config.pszContent = L"Click Retry to download the channel again.";
            else
                config.pszContent = L"Click Retry to download the update again.";
        }
    }

    config.cxWidth = 200;
    config.pfCallback = FinalTaskDialogCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

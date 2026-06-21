/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include "updater.h"

#define UPDATER_TOAST_APP_ID L"SystemInformer"
#define UPDATER_TOAST_TAG    L"SI.Updater"
#define UPDATER_TOAST_GROUP  L"SI.Updater"

#define UPDATER_TOAST_ACTION_DOWNLOAD L"download"
#define UPDATER_TOAST_ACTION_INSTALL  L"install"

_Function_class_(PH_TOAST_CALLBACK2)
VOID NTAPI UpdaterAvailableToastCallback(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_opt_ PCWSTR Arguments,
    _In_ PVOID Context
    );

_Function_class_(PH_TOAST_CALLBACK2)
VOID NTAPI UpdaterProgressToastCallback(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_opt_ PCWSTR Arguments,
    _In_ PVOID Context
    );

_Function_class_(PH_TOAST_CALLBACK2)
VOID NTAPI UpdaterReadyToInstallToastCallback(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_opt_ PCWSTR Arguments,
    _In_ PVOID Context
    );

_Function_class_(PH_TOAST_CALLBACK2)
VOID NTAPI UpdaterFailedToastCallback(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_opt_ PCWSTR Arguments,
    _In_ PVOID Context
    );

static PPH_TOAST UpdaterProgressToast = NULL;
static PPH_TOAST UpdaterAvailableToast = NULL;
static PPH_TOAST UpdaterReadyToInstallToast = NULL;

// Callback context for action-bearing toasts. The toast subsystem invokes V2
// callbacks on its own pump thread (fire-and-forget); this context carries the
// owned updater reference and the toast handle so the callback is the sole
// releaser. Handled makes the callback one-shot in case the runtime delivers
// both an Activated and a subsequent Dismissed event.
typedef struct _UPDATER_TOAST_CONTEXT
{
    PPH_UPDATER_CONTEXT UpdaterContext;
    PPH_TOAST Toast;
    volatile LONG Handled;
} UPDATER_TOAST_CONTEXT, *PUPDATER_TOAST_CONTEXT;

BOOLEAN UpdaterToastActionMatches(
    _In_opt_ PCWSTR Arguments,
    _In_ PCWSTR Action
    )
{
    PH_STRINGREF arguments;

    if (!Arguments)
        return FALSE;

    PhInitializeStringRefLongHint(&arguments, Arguments);
    return PhFindStringInStringRefZ(&arguments, Action, TRUE) != SIZE_MAX;
}


_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI UpdaterToastDownloadThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT updaterContext = (PPH_UPDATER_CONTEXT)Parameter;

    if (!UpdaterShowProgressToast(updaterContext))
    {
        updaterContext->ToastMode = FALSE;
        ShowUpdateDialog(updaterContext);
    }

    PhDereferenceObject(updaterContext);
    return STATUS_SUCCESS;
}

VOID UpdaterLaunchInstaller(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    // Launch the cached installer; mirrors the IDYES path in
    // FinalTaskDialogCallbackProc (page5.c). NULL parent because there is
    // no active dialog window in the toast flow.
    if (!NT_SUCCESS(UpdateShellExecute(Context, NULL)))
    {
        // Re-emit the ready-to-install toast on failure so the user can
        // try again. UpdateShellExecute already surfaces UAC errors.
    }
}

VOID UpdaterUpdateProgressToast(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_opt_ PCWSTR StatusText
    )
{
    PPH_STRING valueString = NULL;
    PPH_STRING statusString = NULL;
    double value;
    ULONG64 total;
    ULONG64 downloaded;
    ULONG64 bitsPerSecond;

    if (!Context->ToastMode || !UpdaterProgressToast)
        return;

    total = (ULONG64)InterlockedCompareExchange64(&Context->ProgressTotal, 0, 0);
    downloaded = (ULONG64)InterlockedCompareExchange64(&Context->ProgressDownloaded, 0, 0);
    bitsPerSecond = (ULONG64)InterlockedCompareExchange64(&Context->ProgressBitsPerSecond, 0, 0);

    if (total)
        value = (double)downloaded / (double)total;
    else
        value = 0.0;

    if (Context->ProgressFinalizing && value >= 1.0)
        value = 0.9999;

    if (StatusText)
    {
        statusString = PhCreateString(StatusText);
    }
    else
    {
        PPH_STRING speedString;

        speedString = PhFormatSize(bitsPerSecond, ULONG_MAX);
        statusString = PhFormatString(L"Speed: %s/s", PhGetStringOrEmpty(speedString));
        if (speedString)
            PhDereferenceObject(speedString);
    }

    if (total)
    {
        PPH_STRING downloadedString;
        PPH_STRING totalString;
        ULONG64 percent;

        percent = (downloaded * 100) / total;
        downloadedString = PhFormatSize(downloaded, ULONG_MAX);
        totalString = PhFormatSize(total, ULONG_MAX);

        if (!downloadedString || !totalString)
        {
            if (downloadedString)
                PhDereferenceObject(downloadedString);
            if (totalString)
                PhDereferenceObject(totalString);
            goto CleanupExit;
        }

        valueString = PhFormatString(
            L"%s of %s (%I64u%%)",
            PhGetStringOrEmpty(downloadedString),
            PhGetStringOrEmpty(totalString),
            percent
            );
        PhDereferenceObject(downloadedString);
        PhDereferenceObject(totalString);
    }
    else
    {
        valueString = PhCreateString(L"Downloaded: ~ of ~");
    }

    if (!valueString || !statusString)
        goto CleanupExit;

    PhUpdateToastProgress(
        UpdaterProgressToast,
        value,
        PhGetString(valueString),
        PhGetString(statusString),
        NULL
        );

CleanupExit:
    if (valueString)
        PhDereferenceObject(valueString);
    if (statusString)
        PhDereferenceObject(statusString);
}

VOID UpdaterHideActiveToasts(
    VOID
    )
{
    if (UpdaterAvailableToast)
    {
        PhHideToast(UpdaterAvailableToast);
        PhReleaseToast(UpdaterAvailableToast);
        UpdaterAvailableToast = NULL;
    }

    if (UpdaterProgressToast)
    {
        PhHideToast(UpdaterProgressToast);
        PhReleaseToast(UpdaterProgressToast);
        UpdaterProgressToast = NULL;
    }

    if (UpdaterReadyToInstallToast)
    {
        PhHideToast(UpdaterReadyToInstallToast);
        PhReleaseToast(UpdaterReadyToInstallToast);
        UpdaterReadyToInstallToast = NULL;
    }
}

_Function_class_(PH_TOAST_CALLBACK2)
VOID NTAPI UpdaterAvailableToastCallback(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_opt_ PCWSTR Arguments,
    _In_ PVOID Context
    )
{
    PUPDATER_TOAST_CONTEXT toastContext = (PUPDATER_TOAST_CONTEXT)Context;
    PPH_UPDATER_CONTEXT updaterContext;

    if (!toastContext)
        return;

    // One-shot: the runtime may deliver Activated (non-terminal in v2) followed
    // by Dismissed. Only the first event performs cleanup.
    if (InterlockedExchange(&toastContext->Handled, 1) != 0)
        return;

    updaterContext = toastContext->UpdaterContext;

    // Action with no arguments means the user clicked the toast body. Open the
    // existing updater dialog using the update data retained by this context.
    if (Reason == PhToastReasonAction && !Arguments)
    {
        InterlockedCompareExchangePointer((PVOID volatile*)&UpdaterAvailableToast, NULL, toastContext->Toast);

        if (toastContext->Toast)
            PhReleaseToast(toastContext->Toast);

        PhFree(toastContext);

        updaterContext->ToastMode = FALSE;
        ShowUpdateDialog(updaterContext);
        return;
    }

    // The download action button starts the update download in the background.
    if (Reason == PhToastReasonAction &&
        UpdaterToastActionMatches(Arguments, UPDATER_TOAST_ACTION_DOWNLOAD))
    {
        PhReferenceObject(updaterContext); // released by UpdaterToastDownloadThread

        if (!NT_SUCCESS(PhCreateThread2(UpdaterToastDownloadThread, updaterContext)))
        {
            PhDereferenceObject(updaterContext); // undo the thread reference

            // Release the toast/context here, then hand our owned reference to
            // ShowUpdateDialog (which consumes exactly one reference).
            InterlockedCompareExchangePointer((PVOID volatile*)&UpdaterAvailableToast, NULL, toastContext->Toast);

            if (toastContext->Toast)
                PhReleaseToast(toastContext->Toast);

            PhFree(toastContext);

            updaterContext->ToastMode = FALSE;
            ShowUpdateDialog(updaterContext);
            return;
        }
    }

    // Dismissed, failed, and unknown action events do not start an update. This
    // is the sole releaser for those paths: drop the toast handle (also unhooks
    // the handlers) and the owned updater reference, then free the context.
    InterlockedCompareExchangePointer((PVOID volatile*)&UpdaterAvailableToast, NULL, toastContext->Toast);

    if (toastContext->Toast)
        PhReleaseToast(toastContext->Toast);

    PhDereferenceObject(updaterContext);
    PhFree(toastContext);
}

_Function_class_(PH_TOAST_CALLBACK2)
VOID NTAPI UpdaterProgressToastCallback(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_opt_ PCWSTR Arguments,
    _In_ PVOID Context
    )
{
    // No buttons on the progress toast -- the only reason we'd receive a
    // callback is Dismissed/Failed. The download keeps running regardless;
    // a ready-to-install toast will appear on completion.
    if (Reason == PhToastReasonAction)
    {
        // Defensive: ignore unknown actions.
        return;
    }
}

_Function_class_(PH_TOAST_CALLBACK2)
VOID NTAPI UpdaterReadyToInstallToastCallback(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_opt_ PCWSTR Arguments,
    _In_ PVOID Context
    )
{
    PUPDATER_TOAST_CONTEXT toastContext = (PUPDATER_TOAST_CONTEXT)Context;
    PPH_UPDATER_CONTEXT updaterContext;

    if (!toastContext)
        return;

    if (InterlockedExchange(&toastContext->Handled, 1) != 0)
        return;

    updaterContext = toastContext->UpdaterContext;

    if (Reason == PhToastReasonAction &&
        UpdaterToastActionMatches(Arguments, UPDATER_TOAST_ACTION_INSTALL))
    {
        UpdaterLaunchInstaller(updaterContext);
    }

    InterlockedCompareExchangePointer((PVOID volatile*)&UpdaterReadyToInstallToast, NULL, toastContext->Toast);

    if (toastContext->Toast)
        PhReleaseToast(toastContext->Toast);

    PhDereferenceObject(updaterContext);
    PhFree(toastContext);
}

_Function_class_(PH_TOAST_CALLBACK2)
VOID NTAPI UpdaterFailedToastCallback(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_opt_ PCWSTR Arguments,
    _In_ PVOID Context
    )
{
    // Fire-and-forget: the failed toast has no actions; nothing to do. The
    // toast subsystem reclaims the handler on Dismissed/Failed.
}

BOOLEAN UpdaterShowAvailableToast(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    static PH_STRINGREF group = PH_STRINGREF_INIT(L"group");
    static PH_STRINGREF tag = PH_STRINGREF_INIT(L"tag");
    PUPDATER_TOAST_CONTEXT toastContext;
    PPH_STRING versionEsc;
    PPH_STRING sizeEsc;
    PPH_STRING xml;
    PH_STRINGREF appIdSr;
    PH_STRINGREF xmlSr;
    HRESULT hr;

    versionEsc = PhEscapeStringForXml(PhGetStringOrEmpty(Context->Version));
    sizeEsc = PhEscapeStringForXml(PhGetStringOrEmpty(Context->SetupFileLength));

    xml = PhFormatString(
        L"<toast launch=\"\" duration=\"long\">"
        L"<visual><binding template=\"ToastGeneric\">"
        L"<text>System Informer - Update Available</text>"
        L"<text>Version %s (download %s)</text>"
        L"</binding></visual>"
        L"<actions>"
        L"<action content=\"Download\" arguments=\"" UPDATER_TOAST_ACTION_DOWNLOAD L"\" activationType=\"foreground\"/>"
        L"</actions>"
        L"</toast>",
        PhGetStringOrEmpty(versionEsc),
        PhGetStringOrEmpty(sizeEsc)
        );

    if (!xml)
    {
        if (versionEsc) PhDereferenceObject(versionEsc);
        if (sizeEsc) PhDereferenceObject(sizeEsc);
        return FALSE;
    }

    PhInitializeStringRefLongHint(&appIdSr, UPDATER_TOAST_APP_ID);
    xmlSr = xml->sr;

    if (UpdaterAvailableToast)
    {
        PhReleaseToast(UpdaterAvailableToast);
        UpdaterAvailableToast = NULL;
    }

    // The callback (invoked on the toast pump thread) owns this reference and
    // the context allocation; it is the sole releaser.
    toastContext = PhAllocateZero(sizeof(UPDATER_TOAST_CONTEXT));
    toastContext->UpdaterContext = Context;
    PhReferenceObject(Context);

    hr = PhShowToastEx2(
        &appIdSr,
        &xmlSr,
        &tag,
        &group,
        50000,
        PhToastPriorityHigh,
        UpdaterAvailableToastCallback,
        toastContext,
        &toastContext->Toast
        );

    PhDereferenceObject(xml);
    if (versionEsc) PhDereferenceObject(versionEsc);
    if (sizeEsc) PhDereferenceObject(sizeEsc);

    if (HR_FAILED(hr))
    {
        PhDereferenceObject(Context);
        PhFree(toastContext);
        return FALSE;
    }

    UpdaterAvailableToast = toastContext->Toast;

    return TRUE;
}

BOOLEAN UpdaterShowProgressToast(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PPH_STRING versionEsc;
    PPH_STRING xml;
    PH_STRINGREF appIdSr;
    PH_STRINGREF xmlSr;
    PH_STRINGREF tagSr;
    PH_STRINGREF groupSr;
    HRESULT hr;

    versionEsc = PhEscapeStringForXml(PhGetStringOrEmpty(Context->Version));

    xml = PhFormatString(
        L"<toast launch=\"\" duration=\"long\">"
        L"<visual><binding template=\"ToastGeneric\">"
        L"<text>Downloading System Informer %s</text>"
        L"<progress title=\"\" status=\"{progressStatus}\" "
        L"value=\"{progressValue}\" valueStringOverride=\"{progressValueString}\"/>"
        L"</binding></visual>"
        L"<actions>"
        L"<action content=\"Close\" arguments=\"dismiss\" activationType=\"system\"/>"
        L"</actions>"
        L"</toast>",
        PhGetStringOrEmpty(versionEsc)
        );

    if (!xml)
    {
        if (versionEsc) PhDereferenceObject(versionEsc);
        return FALSE;
    }

    PhInitializeStringRefLongHint(&appIdSr, UPDATER_TOAST_APP_ID);
    PhInitializeStringRefLongHint(&tagSr, UPDATER_TOAST_TAG);
    PhInitializeStringRefLongHint(&groupSr, UPDATER_TOAST_GROUP);
    xmlSr = xml->sr;

    if (UpdaterProgressToast)
    {
        PhReleaseToast(UpdaterProgressToast);
        UpdaterProgressToast = NULL;
    }
    if (UpdaterReadyToInstallToast)
    {
        PhReleaseToast(UpdaterReadyToInstallToast);
        UpdaterReadyToInstallToast = NULL;
    }

    hr = PhShowToastEx2(
        &appIdSr,
        &xmlSr,
        &tagSr,
        &groupSr,
        0,
        PhToastPriorityDefault,
        UpdaterProgressToastCallback,
        Context,
        &UpdaterProgressToast
        );

    PhDereferenceObject(xml);
    if (versionEsc) PhDereferenceObject(versionEsc);

    if (HR_FAILED(hr))
    {
        return FALSE;
    }

    Context->ToastMode = TRUE;
    UpdaterUpdateProgressToast(Context, L"Starting download...");

    PhReferenceObject(Context);
    PhCreateThread2(UpdateInstallerDownloadThreadStage1, Context);

    return TRUE;
}

BOOLEAN UpdaterShowReadyToInstallToast(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PUPDATER_TOAST_CONTEXT toastContext;
    PPH_STRING versionEsc;
    PPH_STRING xml;
    PH_STRINGREF appIdSr;
    PH_STRINGREF xmlSr;
    PH_STRINGREF tagSr;
    PH_STRINGREF groupSr;
    HRESULT hr;

    versionEsc = PhEscapeStringForXml(PhGetStringOrEmpty(Context->Version));

    xml = PhFormatString(
        L"<toast launch=\"\" scenario=\"reminder\">"
        L"<visual><binding template=\"ToastGeneric\">"
        L"<text>System Informer %s</text>"
        L"<text>Update successfully downloaded and verified.</text>"
        L"</binding></visual>"
        L"<actions>"
        L"<action content=\"Install\" arguments=\"" UPDATER_TOAST_ACTION_INSTALL L"\" activationType=\"foreground\"/>"
        L"<action content=\"Cancel\" arguments=\"dismiss\" activationType=\"system\"/>"
        L"</actions>"
        L"</toast>",
        PhGetStringOrEmpty(versionEsc),
        PhGetStringOrEmpty(versionEsc)
        );

    if (!xml)
    {
        if (versionEsc) PhDereferenceObject(versionEsc);
        return FALSE;
    }

    PhInitializeStringRefLongHint(&appIdSr, UPDATER_TOAST_APP_ID);
    PhInitializeStringRefLongHint(&tagSr, UPDATER_TOAST_TAG);
    PhInitializeStringRefLongHint(&groupSr, UPDATER_TOAST_GROUP);
    xmlSr = xml->sr;

    if (UpdaterProgressToast)
        PhHideToast(UpdaterProgressToast);

    if (UpdaterReadyToInstallToast)
    {
        PhReleaseToast(UpdaterReadyToInstallToast);
        UpdaterReadyToInstallToast = NULL;
    }

    // The callback (invoked on the toast pump thread) owns this reference and
    // the context allocation; it is the sole releaser.
    toastContext = PhAllocateZero(sizeof(UPDATER_TOAST_CONTEXT));
    toastContext->UpdaterContext = Context;
    PhReferenceObject(Context);

    hr = PhShowToastEx2(
        &appIdSr,
        &xmlSr,
        &tagSr,
        &groupSr,
        0,
        PhToastPriorityHigh,
        UpdaterReadyToInstallToastCallback,
        toastContext,
        &toastContext->Toast
        );

    PhDereferenceObject(xml);
    if (versionEsc) PhDereferenceObject(versionEsc);

    if (HR_FAILED(hr))
    {
        PhDereferenceObject(Context);
        PhFree(toastContext);
        return FALSE;
    }

    UpdaterReadyToInstallToast = toastContext->Toast;

    if (UpdaterProgressToast)
    {
        PhReleaseToast(UpdaterProgressToast);
        UpdaterProgressToast = NULL;
    }

    return TRUE;
}

BOOLEAN UpdaterShowFailedToast(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ BOOLEAN HashFailed,
    _In_ BOOLEAN SignatureFailed
    )
{
    PPH_STRING errorEsc = NULL;
    PPH_STRING xml;
    PH_STRINGREF appIdSr;
    PH_STRINGREF xmlSr;
    PH_STRINGREF tagSr;
    PH_STRINGREF groupSr;
    PCWSTR errorText;
    HRESULT hr;

    if (UpdaterAvailableToast)
    {
        PhReleaseToast(UpdaterAvailableToast);
        UpdaterAvailableToast = NULL;
    }

    if (SignatureFailed)
    {
        errorText = L"Signature check failed.";
    }
    else if (HashFailed)
    {
        errorText = L"Hash check failed.";
    }
    else if (Context->UpdateStatus)
    {
        PPH_STRING statusMessage;

        if (statusMessage = PhHttpGetErrorMessage(Context->UpdateStatus))
        {
            errorEsc = PhEscapeStringForXml(PhGetString(statusMessage));
            PhDereferenceObject(statusMessage);
            errorText = PhGetStringOrEmpty(errorEsc);
        }
        else if (statusMessage = PhGetStatusMessage(Context->UpdateStatus, 0))
        {
            errorEsc = PhEscapeStringForXml(PhGetString(statusMessage));
            PhDereferenceObject(statusMessage);
            errorText = PhGetStringOrEmpty(errorEsc);
        }
        else
        {
            errorText = L"Click Check for updates to try again.";
        }
    }
    else
    {
        errorText = L"Click Check for updates to try again.";
    }

    xml = PhFormatString(
        L"<toast launch=\"\" duration=\"long\">"
        L"<visual><binding template=\"ToastGeneric\">"
        L"<text>System Informer update failed</text>"
        L"<text>%s</text>"
        L"</binding></visual>"
        L"</toast>",
        errorText
        );

    if (!xml)
    {
        if (errorEsc)
            PhDereferenceObject(errorEsc);

        return FALSE;
    }

    PhInitializeStringRefLongHint(&appIdSr, UPDATER_TOAST_APP_ID);
    PhInitializeStringRefLongHint(&tagSr, UPDATER_TOAST_TAG);
    PhInitializeStringRefLongHint(&groupSr, UPDATER_TOAST_GROUP);
    xmlSr = xml->sr;

    if (UpdaterProgressToast)
        PhHideToast(UpdaterProgressToast);

    hr = PhShowToastEx2(
        &appIdSr,
        &xmlSr,
        &tagSr,
        &groupSr,
        0,
        PhToastPriorityHigh,
        UpdaterFailedToastCallback,
        NULL,
        NULL
        );

    PhDereferenceObject(xml);
    if (errorEsc)
        PhDereferenceObject(errorEsc);

    if (HR_SUCCESS(hr) && UpdaterProgressToast)
    {
        PhReleaseToast(UpdaterProgressToast);
        UpdaterProgressToast = NULL;
    }

    return HR_SUCCESS(hr);
}

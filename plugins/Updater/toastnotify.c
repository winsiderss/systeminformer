/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2024
 *
 */

// This file implements the integration with Windows Action Center / Toast Notifications.
// It provides the fallback UI when the user has opted to use native notifications
// instead of the traditional TaskDialog flow for updates.

#include "updater.h"

typedef struct _UPDATER_HTTP_DOWNLOAD_CONTEXT
{
    PPH_UPDATER_CONTEXT UpdaterContext;
    HANDLE FileHandle;
    PUPDATER_HASH_CONTEXT HashContext;
} UPDATER_HTTP_DOWNLOAD_CONTEXT, *PUPDATER_HTTP_DOWNLOAD_CONTEXT;

NTSTATUS UpdateInitializeWinHttpDownloadStage3(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PPH_STRING* DownloadHostPath,
    _Out_ PPH_STRING* DownloadUrlPath,
    _Out_ PPH_STRING* UserAgent,
    _Out_ PUPDATER_HTTP_DOWNLOAD_CONTEXT DownloadContext,
    _Out_ PPH_HTTP_DOWNLOAD_OPTIONS Options
    );

_Function_class_(PH_HTTP_EVENT_CALLBACK)
NTSTATUS NTAPI UpdateWinHttpEventCallbackStage4(
    _In_ PHHTTP_EVENT_TYPE Event,
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

NTSTATUS NTAPI UpdateWinHttpTransferCallbackStage5(
    _In_ PH_HTTPDOWNLOAD_EVENT_TYPE Event,
    _In_ PPH_HTTPDOWNLOAD_CALLBACK_CONTEXT Parameter,
    _In_opt_ PVOID Context
    );

NTSTATUS UpdateFinalizeWinHttpDownloadStage6(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Inout_ PUPDATER_HTTP_DOWNLOAD_CONTEXT DownloadContext,
    _Out_ PUPDATER_DOWNLOAD_RESULT Result
    );

VOID UpdateReleaseWinHttpDownloadContext(
    _Inout_ PUPDATER_HTTP_DOWNLOAD_CONTEXT Context
    );

VOID UpdateReleaseWinHttpDownloadStringsStage7(
    _Inout_opt_ PPH_STRING* DownloadHostPath,
    _Inout_opt_ PPH_STRING* DownloadUrlPath,
    _Inout_opt_ PPH_STRING* UserAgent
    );

VOID UpdateSetDialogStatusText(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PCWSTR MainInstruction
    );

VOID UpdateSetDialogInitialProgressText(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PCWSTR MainInstruction
    );

/**
 * \brief Converts an HRESULT to an NTSTATUS.
 * \param Result The HRESULT to convert.
 * \return The converted NTSTATUS.
 */
NTSTATUS UpdateNtStatusFromHresult(
    _In_ HRESULT Result
    )
{
    if (HRESULT_NTSTATUS(Result))
    {
        ClearFlag(Result, FACILITY_NT_BIT);
    }

    return Result;
}

/**
 * \brief Updates the progress state in the updater context.
 * \param Context The updater context.
 * \param TotalLength The total length of the download.
 * \param ReadLength The number of bytes read so far.
 * \param BitsPerSecond The current download speed in bits per second.
 */
VOID UpdateSetProgressState(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ ULONG64 TotalLength,
    _In_ ULONG64 ReadLength,
    _In_ ULONG64 BitsPerSecond
    )
{
    InterlockedExchange64(&Context->ProgressTotal, TotalLength);
    InterlockedExchange64(&Context->ProgressDownloaded, ReadLength);
    InterlockedExchange64(&Context->ProgressBitsPerSecond, BitsPerSecond);

    if (Context->ProgressFinalizing)
        Context->ProgressFinalizing = FALSE;

    UpdaterUpdateProgressToast(Context, NULL);
}

/**
 * \brief Sets the progress state to finalizing.
 * \param Context The updater context.
 * \param MainInstruction The status text to display.
 */
VOID UpdateSetProgressFinalizingState(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PCWSTR MainInstruction
    )
{
    if (Context->ProgressFinalizing)
        return;

    Context->ProgressFinalizing = TRUE;
    UpdaterUpdateProgressToast(Context, MainInstruction);

    if (Context->DialogHandle)
    {
        SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)MainInstruction);
    }
}

/**
 * \brief Thread routine for downloading the installer.
 * \param Parameter The updater context.
 * \return NTSTATUS Successful or errant status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UpdateInstallerDownloadThreadStage1(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)Parameter;
    NTSTATUS status;
    UPDATER_DOWNLOAD_RESULT result;
    ULONG downloadMethod;

    memset(&result, 0, sizeof(result));
    downloadMethod = PhGetIntegerSetting(SETTING_NAME_DOWNLOAD_METHOD);

    switch (downloadMethod)
    {
    case UpdateDownloadMethodAutomatic:
        {
            status = UpdateDownloadFileWithDeliveryOptimization(context, &result);

            if (NT_SUCCESS(status) || context->Cancel ||
                status == STATUS_DATA_CHECKSUM_ERROR || result.HashSuccess || result.SignatureSuccess)
            {
                break;
            }

            status = UpdateDownloadFileWithBits(context, &result);

            if (NT_SUCCESS(status) || context->Cancel ||
                status == STATUS_DATA_CHECKSUM_ERROR || result.HashSuccess || result.SignatureSuccess)
            {
                break;
            }

            status = UpdateDownloadInstallerWithWinHttpStage2(context, &result);
        }
        break;
    default:
    case UpdateDownloadMethodWinHttp:
        {
            status = UpdateDownloadInstallerWithWinHttpStage2(context, &result);
        }
        break;
    case UpdateDownloadMethodBits:
        {
            status = UpdateDownloadFileWithBits(context, &result);
        }
        break;
    case UpdateDownloadMethodDeliveryOptimization:
        {
            status = UpdateDownloadFileWithDeliveryOptimization(context, &result);
        }
        break;
    }

    context->UpdateStatus = status;
    UpdatePostDownloadResult(context, &result);

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}

/**
 * \brief Downloads the installer using WinHTTP.
 * \param Context The updater context.
 * \param Result A pointer to a structure that receives the download results.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS UpdateDownloadInstallerWithWinHttpStage2(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PUPDATER_DOWNLOAD_RESULT Result
    )
{
    NTSTATUS status;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    PPH_STRING userAgent = NULL;
    PH_HTTP_DOWNLOAD_OPTIONS options = { 0 };
    UPDATER_HTTP_DOWNLOAD_CONTEXT downloadContext = { 0 };

    memset(Result, 0, sizeof(UPDATER_DOWNLOAD_RESULT));

    status = UpdateInitializeWinHttpDownloadStage3(
        Context,
        &downloadHostPath,
        &downloadUrlPath,
        &userAgent,
        &downloadContext,
        &options
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhHttpDownloadUrl(Context->SetupFileDownloadUrl, &options);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = UpdateFinalizeWinHttpDownloadStage6(Context, &downloadContext, Result);

CleanupExit:
    UpdateReleaseWinHttpDownloadContext(&downloadContext);
    UpdateReleaseWinHttpDownloadStringsStage7(&downloadHostPath, &downloadUrlPath, &userAgent);

    return status;
}

/**
 * \brief Initializes a WinHTTP download session.
 * \param Context The updater context.
 * \param DownloadHostPath A pointer to a string that receives the host path.
 * \param DownloadUrlPath A pointer to a string that receives the URL path.
 * \param UserAgent A pointer to a string that receives the user agent.
 * \param DownloadContext A pointer to a structure that receives the download context.
 * \param Options A pointer to a structure that receives the download options.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS UpdateInitializeWinHttpDownloadStage3(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PPH_STRING* DownloadHostPath,
    _Out_ PPH_STRING* DownloadUrlPath,
    _Out_ PPH_STRING* UserAgent,
    _Out_ PUPDATER_HTTP_DOWNLOAD_CONTEXT DownloadContext,
    _Out_ PPH_HTTP_DOWNLOAD_OPTIONS Options
    )
{
    NTSTATUS status;
    USHORT port;

    if (Context->SetupFilePath)
    {
        PhDeleteCacheFile(Context->SetupFilePath, FALSE);
        PhClearReference(&Context->SetupFilePath);
    }

    InterlockedExchange64(&Context->ProgressTotal, 0);
    InterlockedExchange64(&Context->ProgressDownloaded, 0);
    InterlockedExchange64(&Context->ProgressBitsPerSecond, 0);

    if (!NT_SUCCESS(status = PhHttpCrackUrl(
        Context->SetupFileDownloadUrl,
        DownloadHostPath,
        DownloadUrlPath,
        &port
        )))
    {
        return status;
    }

    Context->SetupFilePath = UpdateParseDownloadFileName(Context, *DownloadUrlPath);

    if (PhIsNullOrEmptyString(Context->SetupFilePath))
        return STATUS_FILE_HANDLE_REVOKED;

    *UserAgent = PhWinHttpUserAgentString();
    if (!*UserAgent)
        return STATUS_NO_MEMORY;

    memset(DownloadContext, 0, sizeof(UPDATER_HTTP_DOWNLOAD_CONTEXT));
    DownloadContext->UpdaterContext = Context;

    memset(Options, 0, sizeof(PH_HTTP_DOWNLOAD_OPTIONS));
    Options->UserAgent = *UserAgent;
    Options->RequestFlags = PH_HTTP_FLAG_SECURE;
    Options->ProtocolFlags = PH_HTTP_PROTOCOL_FLAG_HTTP2;
    Options->ProtocolTimeout = 5000;
    Options->DisableFeatures = PH_HTTP_FEATURE_KEEP_ALIVE;
    Options->EventCallback = UpdateWinHttpEventCallbackStage4;
    Options->DownloadCallback = UpdateWinHttpTransferCallbackStage5;
    Options->Context = DownloadContext;

    return STATUS_SUCCESS;
}

/**
 * \brief Callback for WinHTTP events.
 * \param Event The event type.
 * \param Parameter Event-specific parameter.
 * \param Context The download context.
 * \return NTSTATUS Successful or errant status.
 */
_Function_class_(PH_HTTP_EVENT_CALLBACK)
NTSTATUS NTAPI UpdateWinHttpEventCallbackStage4(
    _In_ PHHTTP_EVENT_TYPE Event,
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PUPDATER_HTTP_DOWNLOAD_CONTEXT downloadContext = Context;
    PPH_UPDATER_CONTEXT updater;

    if (!downloadContext)
        return STATUS_INVALID_PARAMETER;

    updater = downloadContext->UpdaterContext;

    if (!updater)
        return STATUS_INVALID_PARAMETER;

    if (updater->Cancel)
        return STATUS_CANCELLED;

    switch (Event)
    {
    case PHHTTP_EVENT_INITIALIZING:
        UpdateSetDialogStatusText(updater, L"Initializing download request...");
        break;
    case PHHTTP_EVENT_CONNECTING:
        UpdateSetDialogStatusText(updater, L"Connecting...");
        break;
    case PHHTTP_EVENT_SENDING_REQUEST:
        UpdateSetDialogStatusText(updater, L"Sending download request...");
        break;
    case PHHTTP_EVENT_RECEIVING_RESPONSE:
        UpdateSetDialogStatusText(updater, L"Waiting for response...");
        break;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Callback for WinHTTP transfer events.
 * \param Event The event type.
 * \param Parameter Event-specific parameter.
 * \param Context The download context.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI UpdateWinHttpTransferCallbackStage5(
    _In_ PH_HTTPDOWNLOAD_EVENT_TYPE Event,
    _In_ PPH_HTTPDOWNLOAD_CALLBACK_CONTEXT Parameter,
    _In_opt_ PVOID Context
    )
{
    PUPDATER_HTTP_DOWNLOAD_CONTEXT downloadContext = Context;
    PPH_UPDATER_CONTEXT updater;
    NTSTATUS status;

    if (!downloadContext)
        return STATUS_INVALID_PARAMETER;

    updater = downloadContext->UpdaterContext;

    if (!updater)
        return STATUS_INVALID_PARAMETER;

    if (updater->Cancel)
        return STATUS_CANCELLED;

    switch (Event)
    {
    case PH_HTTPDOWNLOAD_EVENT_BEGIN:
        {
            LARGE_INTEGER allocationSize;
            PPH_STRING string;

            string = PhFormatString(L"Downloading release %s...", PhGetStringOrEmpty(updater->Version));
            UpdateSetDialogInitialProgressText(updater, string->Buffer);
            PhDereferenceObject(string);

            if (Parameter->TotalLength)
            {
                allocationSize.QuadPart = Parameter->TotalLength;
            }

            status = PhCreateFileWin32Ex(
                &downloadContext->FileHandle,
                PhGetString(updater->SetupFilePath),
                FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                Parameter->TotalLength ? &allocationSize : NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ ,
                FILE_OVERWRITE_IF,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL
                );

            if (!NT_SUCCESS(status))
                return status;

            return UpdaterInitializeHashForContext(&downloadContext->HashContext, updater);
        }
        break;
    case PH_HTTPDOWNLOAD_EVENT_DATA:
        {
            ULONG bytesWritten = 0;

            status = UpdaterHashDataForContext(
                downloadContext->HashContext,
                Parameter->Buffer,
                Parameter->BufferLength
                );

            if (!NT_SUCCESS(status))
                return status;

            status = PhWriteFile(
                downloadContext->FileHandle,
                Parameter->Buffer,
                Parameter->BufferLength,
                NULL,
                &bytesWritten
                );

            if (!NT_SUCCESS(status))
                return status;

            if (bytesWritten != Parameter->BufferLength)
                return STATUS_DATA_CHECKSUM_ERROR;

            return STATUS_SUCCESS;
        }
        break;
    case PH_HTTPDOWNLOAD_EVENT_PROGRESS:
        {
            UpdateSetProgressState(
                updater,
                Parameter->TotalLength,
                Parameter->ReadLength,
                Parameter->BitsPerSecond
                );
        }
        return STATUS_SUCCESS;
    case PH_HTTPDOWNLOAD_EVENT_END:
    default:
        return STATUS_SUCCESS;
    }
}

/**
 * \brief Finalizes a WinHTTP download.
 * \param Context The updater context.
 * \param DownloadContext The download context.
 * \param Result A pointer to a structure that receives the download results.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS UpdateFinalizeWinHttpDownloadStage6(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Inout_ PUPDATER_HTTP_DOWNLOAD_CONTEXT DownloadContext,
    _Out_ PUPDATER_DOWNLOAD_RESULT Result
    )
{
    NTSTATUS status;
    HANDLE setupFileHandle;

    if (NT_SUCCESS(status = UpdaterVerifyHashForContext(
        DownloadContext->HashContext,
        Context->SetupFileHash
        )))
    {
        Result->HashSuccess = TRUE;

        if (NT_SUCCESS(status = UpdaterVerifySignatureForContext(
            DownloadContext->HashContext,
            Context->SetupFileSignature
            )))
        {
            Result->SignatureSuccess = TRUE;
        }
    }

    if (!(Result->HashSuccess && Result->SignatureSuccess))
        return STATUS_DATA_CHECKSUM_ERROR;

    status = PhReOpenFile(
        &setupFileHandle,
        DownloadContext->FileHandle,
        FILE_GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    Context->SetupFileHandle = setupFileHandle;
    Result->DownloadSuccess = TRUE;

    return STATUS_SUCCESS;
}

EXTERN_C NTSTATUS UpdateVerifyCacheFileSignature(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PUPDATER_DOWNLOAD_RESULT Result
    )
{
    NTSTATUS status;
    HANDLE fileHandle = NULL;
    HANDLE setupFileHandle = NULL;
    PUPDATER_HASH_CONTEXT hashContext = NULL;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER fileSize;
    ULONG64 bytesRemaining;
    PBYTE buffer = NULL;
    ULONG bufferLength;

    memset(Result, 0, sizeof(UPDATER_DOWNLOAD_RESULT));

    status = PhCreateFileWin32Ex(
        &fileHandle,
        PhGetString(Context->SetupFilePath),
        FILE_GENERIC_READ,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = UpdaterInitializeHashForContext(&hashContext, Context);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    bufferLength = PAGE_SIZE * 2;
    buffer = PhAllocateSafe(bufferLength);

    if (!buffer)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    status = PhGetFileSize(fileHandle, &fileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    bytesRemaining = fileSize.QuadPart;
    while (bytesRemaining)
    {
        status = NtReadFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &iosb,
            buffer,
            bufferLength,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;

        status = UpdaterHashDataForContext(hashContext, buffer, (ULONG)iosb.Information);
        bytesRemaining -= (ULONG)iosb.Information;

        if (!NT_SUCCESS(status))
            break;
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (NT_SUCCESS(status = UpdaterVerifyHashForContext(hashContext, Context->SetupFileHash)))
    {
        Result->HashSuccess = TRUE;

        if (NT_SUCCESS(status = UpdaterVerifySignatureForContext(hashContext, Context->SetupFileSignature)))
        {
            Result->SignatureSuccess = TRUE;
        }
    }

    if (!(Result->HashSuccess && Result->SignatureSuccess))
    {
        status = STATUS_DATA_CHECKSUM_ERROR;
        goto CleanupExit;
    }

    status = PhReOpenFile(
        &setupFileHandle,
        fileHandle,
        FILE_GENERIC_READ,
        FILE_SHARE_READ,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    Context->SetupFileHandle = setupFileHandle;
    setupFileHandle = NULL;
    Result->DownloadSuccess = TRUE;

CleanupExit:

    if (buffer)
        PhFree(buffer);
    if (hashContext)
        UpdaterDestroyHashForContext(hashContext);
    if (setupFileHandle)
        NtClose(setupFileHandle);
    if (fileHandle)
        NtClose(fileHandle);

    return status;
}

/**
 * \brief Releases resources used by a WinHTTP download context.
 * \param Context The download context.
 */
VOID UpdateReleaseWinHttpDownloadContext(
    _Inout_ PUPDATER_HTTP_DOWNLOAD_CONTEXT Context
    )
{
    if (Context->HashContext)
    {
        UpdaterDestroyHashForContext(Context->HashContext);
        Context->HashContext = NULL;
    }

    if (Context->FileHandle)
    {
        NtClose(Context->FileHandle);
        Context->FileHandle = NULL;
    }
}

/**
 * \brief Releases strings used by a WinHTTP download session.
 * \param DownloadHostPath The host path string.
 * \param DownloadUrlPath The URL path string.
 * \param UserAgent The user agent string.
 */
VOID UpdateReleaseWinHttpDownloadStringsStage7(
    _Inout_opt_ PPH_STRING* DownloadHostPath,
    _Inout_opt_ PPH_STRING* DownloadUrlPath,
    _Inout_opt_ PPH_STRING* UserAgent
    )
{
    if (DownloadHostPath && *DownloadHostPath)
    {
        PhDereferenceObject(*DownloadHostPath);
        *DownloadHostPath = NULL;
    }

    if (DownloadUrlPath && *DownloadUrlPath)
    {
        PhDereferenceObject(*DownloadUrlPath);
        *DownloadUrlPath = NULL;
    }

    if (UserAgent && *UserAgent)
    {
        PhDereferenceObject(*UserAgent);
        *UserAgent = NULL;
    }
}

/**
 * \brief Updates the status text in the UI.
 * \param Context The updater context.
 * \param MainInstruction The status text.
 */
VOID UpdateSetDialogStatusText(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PCWSTR MainInstruction
    )
{
    if (Context->DialogHandle)
        SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)MainInstruction);
    else if (Context->ToastMode)
        UpdaterUpdateProgressToast(Context, MainInstruction);
}

/**
 * \brief Sets the initial progress text in the UI.
 * \param Context The updater context.
 * \param MainInstruction The status text.
 */
VOID UpdateSetDialogInitialProgressText(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PCWSTR MainInstruction
    )
{
    if (Context->DialogHandle)
    {
        SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)MainInstruction);
        SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"Downloaded: ~ of ~ (0%)\r\nSpeed: ~ KB/s");
    }
    else if (Context->ToastMode)
    {
        UpdaterUpdateProgressToast(Context, MainInstruction);
    }
}

/**
 * \brief Posts the download result and updates the UI.
 * \param Context The updater context.
 * \param Result The download result.
 */
VOID UpdatePostDownloadResult(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PUPDATER_DOWNLOAD_RESULT Result
    )
{
    if (Result->DownloadSuccess && Result->HashSuccess && Result->SignatureSuccess)
    {
        if (Context->DialogHandle)
        {
            PostMessage(Context->DialogHandle, PH_SHOWINSTALL, 0, 0);
        }
        else if (Context->ToastMode)
        {
            if (!UpdaterShowReadyToInstallToast(Context))
            {
                Context->ToastMode = FALSE;

                PhReferenceObject(Context);
                ShowUpdateDialog(Context);
            }
        }
    }
    else
    {
        if (Context->SetupFilePath)
        {
            PhDeleteCacheFile(Context->SetupFilePath, FALSE);
        }

        if (Context->DialogHandle)
        {
            if (Result->SignatureSuccess)
                PostMessage(Context->DialogHandle, PH_SHOWERROR, TRUE, FALSE);
            else if (Result->HashSuccess)
                PostMessage(Context->DialogHandle, PH_SHOWERROR, FALSE, TRUE);
            else
                PostMessage(Context->DialogHandle, PH_SHOWERROR, FALSE, FALSE);
        }
        else if (Context->ToastMode)
        {
            BOOLEAN hashFailed;
            BOOLEAN signatureFailed;

            signatureFailed = Result->HashSuccess && !Result->SignatureSuccess;
            hashFailed = !Result->HashSuccess;

            if (!UpdaterShowFailedToast(Context, hashFailed, signatureFailed))
            {
                Context->ToastMode = FALSE;

                PhReferenceObject(Context);
                ShowUpdateDialog(Context);
            }
        }
    }
}

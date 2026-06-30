/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2026
 *
 */

// This file contains the implementation for downloading updates using the Windows
// Background Intelligent Transfer Service (BITS) COM API. It manages BITS jobs,
// tracks progress via `IBackgroundCopyCallback`, and handles transient errors and
// job completion.

#include "updater.h"

#include <bits.h>
#include <combaseapi.h>
#include <memory>
#include <type_traits>

// 4991d34b-80a1-4291-83b6-3328366b9097
DEFINE_GUID(CLSID_BackgroundCopyManager, 0x4991d34b, 0x80a1, 0x4291, 0x83, 0xb6, 0x33, 0x28, 0x36, 0x6b, 0x90, 0x97);

/**
 * \brief COM object releaser for BITS interfaces.
 * \tparam T The interface type.
 */
template <typename T>
struct BitsComReleaser
{
    void operator()(T* pointer) const noexcept
    {
        if (pointer)
            pointer->Release();
    }
};

/**
 * \brief PhObject deleter for smart pointers.
 */
struct BitsPhObjectDeleter
{
    template <typename T>
    void operator()(T* pointer) const noexcept
    {
        if (pointer)
            PhDereferenceObject(pointer);
    }
};

/** \brief COM smart pointer for BITS. */
template <typename T>
using bits_com_ptr = std::unique_ptr<T, BitsComReleaser<T>>;
/** \brief Ph string smart pointer for BITS. */
using bits_ph_string = std::unique_ptr<std::remove_pointer_t<PPH_STRING>, BitsPhObjectDeleter>;

/**
 * \brief Callback implementation for BITS job events.
 */
class UpdateBitsCallback final : public IBackgroundCopyCallback
{
public:
    UpdateBitsCallback(const UpdateBitsCallback&) = delete;
    UpdateBitsCallback(UpdateBitsCallback&&) = delete;
    UpdateBitsCallback& operator=(const UpdateBitsCallback&) = delete;
    UpdateBitsCallback& operator=(UpdateBitsCallback&&) = delete;

    /**
     * \brief Initializes a new instance of the UpdateBitsCallback class.
     * \param context The updater context.
     */
    explicit UpdateBitsCallback(
        _In_ PPH_UPDATER_CONTEXT context
        ) noexcept
        : RefCount(1),
          CompletionEvent(nullptr),
          ErrorCode(S_OK),
          Context(context)
    {
        PhQuerySystemTime(&TimeStart);
    }

    ~UpdateBitsCallback()
    {
        if (CompletionEvent)
        {
            NtClose(CompletionEvent);
        }
    }

    /**
     * \brief Initializes the callback instance.
     * \return NTSTATUS Successful or errant status.
     */
    NTSTATUS Initialize() noexcept
    {
        return PhCreateEvent(
            &CompletionEvent,
            EVENT_ALL_ACCESS,
            NotificationEvent,
            FALSE
            );
    }

    /**
     * \brief Gets the completion event handle.
     * \return The handle to the completion event.
     */
    HANDLE GetCompletionEvent() const noexcept
    {
        return CompletionEvent;
    }

    /**
     * \brief Gets the error code from the BITS job.
     * \return The HRESULT error code.
     */
    HRESULT GetErrorCode() const noexcept
    {
        return ErrorCode;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(
        _In_ REFIID riid,
        _Outptr_ VOID** object
        ) override
    {
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, __uuidof(IBackgroundCopyCallback)))
        {
            *object = static_cast<IBackgroundCopyCallback*>(this);
            AddRef();
            return S_OK;
        }

        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&RefCount);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG refCount = InterlockedDecrement(&RefCount);

        if (refCount == 0)
            delete this;

        return refCount;
    }

    HRESULT STDMETHODCALLTYPE JobTransferred(
        _In_ IBackgroundCopyJob* job
        ) override
    {
        BG_JOB_PROGRESS progress;

        if (job && HR_SUCCESS(job->GetProgress(&progress)) && progress.BytesTotal != BG_SIZE_UNKNOWN)
            UpdateProgress(progress, FALSE);

        NtSetEvent(CompletionEvent, nullptr);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE JobError(
        _In_ IBackgroundCopyJob* job,
        _In_ IBackgroundCopyError* error
        ) override
    {
        BG_ERROR_CONTEXT errorContext;

        ErrorCode = E_FAIL;

        if (error)
            error->GetError(&errorContext, &ErrorCode);

        NtSetEvent(CompletionEvent, nullptr);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE JobModification(
        _In_ IBackgroundCopyJob* job,
        _In_ DWORD reserved
        ) override
    {
        BG_JOB_PROGRESS progress;

        if (job && HR_SUCCESS(job->GetProgress(&progress)) && progress.BytesTotal != BG_SIZE_UNKNOWN)
            UpdateProgress(progress, TRUE);

        return S_OK;
    }

private:
    /**
     * \brief Updates the progress of the download.
     * \param progress The job progress information.
     * \param Finalizing TRUE if the download is finalizing, FALSE otherwise.
     */
    VOID UpdateProgress(
        _In_ const BG_JOB_PROGRESS& progress,
        _In_ BOOLEAN Finalizing
        ) const noexcept
    {
        LARGE_INTEGER timeNow;
        ULONG64 timeTicks;
        ULONG64 bytesPerSecond;

        PhQuerySystemTime(&timeNow);
        timeTicks = (timeNow.QuadPart - TimeStart.QuadPart) / PH_TICKS_PER_SEC;
        bytesPerSecond = timeTicks ? progress.BytesTransferred / timeTicks : 0;

        if (Finalizing && progress.BytesTotal != 0 && progress.BytesTransferred >= progress.BytesTotal)
            UpdateSetProgressFinalizingState(Context, L"Finalizing BITS download...");
        else
            UpdateSetProgressState(Context, progress.BytesTotal, progress.BytesTransferred, bytesPerSecond);
    }

    volatile LONG RefCount;
    HANDLE CompletionEvent;
    LARGE_INTEGER TimeStart;
    HRESULT ErrorCode;
    PPH_UPDATER_CONTEXT Context;
};

/**
 * \brief Updates the status text in the UI based on BITS job state.
 * \param Context The updater context.
 * \param State The current BITS job state.
 */
VOID UpdateBitsStatusText(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ BG_JOB_STATE State
    )
{
    switch (State)
    {
    case BG_JOB_STATE_QUEUED:
        {
            if (Context->DialogHandle)
                SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, reinterpret_cast<LPARAM>(L"Queued BITS download..."));
            else if (Context->ToastMode)
                UpdaterUpdateProgressToast(Context, L"Queued BITS download...");
        }
        break;
    case BG_JOB_STATE_CONNECTING:
        {
            if (Context->DialogHandle)
                SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, reinterpret_cast<LPARAM>(L"Connecting with BITS..."));
            else if (Context->ToastMode)
                UpdaterUpdateProgressToast(Context, L"Connecting with BITS...");
        }
        break;
    case BG_JOB_STATE_TRANSFERRING:
        {
            if (Context->DialogHandle)
                SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, reinterpret_cast<LPARAM>(L"Downloading with BITS..."));
            else if (Context->ToastMode)
                UpdaterUpdateProgressToast(Context, L"Downloading with BITS...");
        }
        break;
    case BG_JOB_STATE_TRANSIENT_ERROR:
        {
            if (Context->DialogHandle)
                SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, reinterpret_cast<LPARAM>(L"BITS download retry pending..."));
            else if (Context->ToastMode)
                UpdaterUpdateProgressToast(Context, L"BITS download retry pending...");
        }
        break;
    }
}

/**
 * \brief Calculates the download speed in bytes per second.
 * \param StartTime The time when the download started.
 * \param BytesTransferred The number of bytes transferred so far.
 * \return The calculated speed in bytes per second.
 */
ULONG64 UpdateBitsBytesPerSecond(
    _In_ PLARGE_INTEGER StartTime,
    _In_ ULONG64 BytesTransferred
    )
{
    LARGE_INTEGER timeNow;
    ULONG64 timeTicks;

    PhQuerySystemTime(&timeNow);
    timeTicks = (timeNow.QuadPart - StartTime->QuadPart) / PH_TICKS_PER_SEC;

    return timeTicks ? BytesTransferred / timeTicks : 0;
}

/**
 * \brief Downloads a file using Background Intelligent Transfer Service (BITS).
 * \param Context The updater context.
 * \param Result A pointer to a structure that receives the download results.
 * \return NTSTATUS Successful or errant status.
 */
EXTERN_C NTSTATUS UpdateDownloadFileWithBits(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PUPDATER_DOWNLOAD_RESULT Result
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HRESULT result;
    BOOLEAN jobCreated = FALSE;
    IBackgroundCopyManager* rawBitsManager = NULL;
    IBackgroundCopyJob* rawBitsJob = NULL;
    bits_com_ptr<IBackgroundCopyManager> bitsManager;
    bits_com_ptr<IBackgroundCopyJob> bitsJob;
    GUID jobId;
    bits_ph_string downloadHostPath;
    bits_ph_string downloadUrlPath;
    PPH_STRING rawDownloadHostPath = NULL;
    PPH_STRING rawDownloadUrlPath = NULL;
    USHORT downloadPort;
    LARGE_INTEGER startTime;
    BG_JOB_STATE previousState = BG_JOB_STATE_QUEUED;
    IClassFactory* rawManagerClassFactory = nullptr;
    bits_com_ptr<IClassFactory> bitsManagerClassFactory;
    UpdateBitsCallback* callback = nullptr;
    NTSTATUS callbackStatus;

    if (Context->DialogHandle)
    {
        SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing BITS download...");
        SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"Downloaded: ~ of ~ (0%)\r\nSpeed: ~ KB/s");
    }
    else if (Context->ToastMode)
    {
        UpdaterUpdateProgressToast(Context, L"Initializing BITS download...");
    }

    memset(Result, 0, sizeof(UPDATER_DOWNLOAD_RESULT));

    if (Context->SetupFilePath)
    {
        PhDeleteCacheFile(Context->SetupFilePath, FALSE);
        PhClearReference(&Context->SetupFilePath);
    }

    InterlockedExchange64(&Context->ProgressTotal, 0);
    InterlockedExchange64(&Context->ProgressDownloaded, 0);
    InterlockedExchange64(&Context->ProgressBitsPerSecond, 0);

    status = PhHttpCrackUrl(
        Context->SetupFileDownloadUrl,
        &rawDownloadHostPath,
        &rawDownloadUrlPath,
        &downloadPort
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    downloadHostPath.reset(rawDownloadHostPath);
    downloadUrlPath.reset(rawDownloadUrlPath);

    Context->SetupFilePath = UpdateParseDownloadFileName(Context, downloadUrlPath.get());

    if (PhIsNullOrEmptyString(Context->SetupFilePath))
    {
        status = STATUS_FILE_HANDLE_REVOKED;
        goto CleanupExit;
    }

    result = CoGetClassObject(
        CLSID_BackgroundCopyManager,
        CLSCTX_LOCAL_SERVER,
        nullptr,
        IID_PPV_ARGS(&rawManagerClassFactory)
        );

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    bitsManagerClassFactory.reset(rawManagerClassFactory);
    result = bitsManagerClassFactory->CreateInstance(
        nullptr,
        IID_PPV_ARGS(&rawBitsManager)
        );

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    bitsManager.reset(rawBitsManager);

    result = bitsManager->CreateJob(
        L"System Informer Update",
        BG_JOB_TYPE_DOWNLOAD,
        &jobId,
        &rawBitsJob
        );

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    bitsJob.reset(rawBitsJob);
    jobCreated = TRUE;

    callback = new UpdateBitsCallback(Context);

    if (!callback)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    callbackStatus = callback->Initialize();

    if (!NT_SUCCESS(callbackStatus))
    {
        status = callbackStatus;
        goto CleanupExit;
    }

    result = bitsJob->SetNotifyInterface(callback);

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    result = bitsJob->SetNotifyFlags(BG_NOTIFY_JOB_TRANSFERRED | BG_NOTIFY_JOB_ERROR | BG_NOTIFY_JOB_MODIFICATION);

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    bitsJob->SetPriority(BG_JOB_PRIORITY_FOREGROUND);
    bitsJob->SetNoProgressTimeout(120);

    result = bitsJob->AddFile(
        PhGetString(Context->SetupFileDownloadUrl),
        PhGetString(Context->SetupFilePath)
        );

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    result = bitsJob->Resume();

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    PhQuerySystemTime(&startTime);

    while (TRUE)
    {
        BG_JOB_STATE state;
        BG_JOB_PROGRESS progress;
        LARGE_INTEGER timeout;

        if (Context->Cancel)
        {
            bitsJob->Cancel();
            jobCreated = FALSE;
            status = STATUS_CANCELLED;
            goto CleanupExit;
        }

        result = bitsJob->GetState(&state);

        if (HR_FAILED(result))
        {
            status = UpdateNtStatusFromHresult(result);
            goto CleanupExit;
        }

        if (state != previousState)
        {
            UpdateBitsStatusText(Context, state);
            previousState = state;
        }

        result = bitsJob->GetProgress(&progress);

        if (HR_SUCCESS(result) && progress.BytesTotal != BG_SIZE_UNKNOWN)
        {
            if (state != BG_JOB_STATE_TRANSFERRED &&
                progress.BytesTotal != 0 &&
                progress.BytesTransferred >= progress.BytesTotal)
            {
                UpdateSetProgressFinalizingState(Context, L"Finalizing BITS download...");
            }
            else
            {
                UpdateSetProgressState(
                    Context,
                    progress.BytesTotal,
                    progress.BytesTransferred,
                    UpdateBitsBytesPerSecond(&startTime, progress.BytesTransferred)
                    );
            }
        }

        switch (state)
        {
        case BG_JOB_STATE_TRANSFERRED:
            {
                result = bitsJob->Complete();
                jobCreated = FALSE;

                if (HR_FAILED(result))
                {
                    status = UpdateNtStatusFromHresult(result);
                    goto CleanupExit;
                }

                status = UpdateVerifyCacheFileSignature(Context, Result);
            }
            goto CleanupExit;
        case BG_JOB_STATE_ERROR:
            {
                IBackgroundCopyError* bitsError;
                HRESULT errorCode = E_FAIL;
                BG_ERROR_CONTEXT errorContext;

                result = bitsJob->GetError(&bitsError);

                if (HR_SUCCESS(result) && bitsError)
                    bitsError->GetError(&errorContext, &errorCode);

                status = UpdateNtStatusFromHresult(errorCode);
            }
            goto CleanupExit;
        case BG_JOB_STATE_CANCELLED:
            jobCreated = FALSE;
            status = STATUS_CANCELLED;
            goto CleanupExit;
        }

        result = NtWaitForSingleObject(
            callback->GetCompletionEvent(),
            FALSE,
            PhTimeoutFromMilliseconds(&timeout, 200)
            );

        if (result == WAIT_FAILED)
        {
            status = PhGetLastWin32ErrorAsNtStatus();
            goto CleanupExit;
        }
    }

CleanupExit:

    if (bitsJob)
    {
        bitsJob->SetNotifyFlags(0);
        bitsJob->SetNotifyInterface(nullptr);
    }

    if (callback)
        callback->Release();

    if (jobCreated && bitsJob)
        bitsJob->Cancel();

    return status;
}

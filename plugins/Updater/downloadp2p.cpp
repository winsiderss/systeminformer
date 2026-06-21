/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2024-2026
 *
 */

//
// This file implements Delivery Optimization (DO) for downloading updates.
// Delivery Optimization (DO) is enabled by default on Windows 10 and Windows 11,
// and the primary peer-to-peer (P2P) distribution mechanism for Windows Updates, 
// Microsoft Store apps, and Windows Defender definitions.
//

#include "updater.h"

#include <deliveryoptimization.h>
#include <memory>
#include <type_traits>

// {5B99FA76-721C-423C-ADAC-56D03C8A8007}
DEFINE_GUID(CLSID_IDOManager, 0x5b99fa76, 0x721c, 0x423c, 0xad, 0xac, 0x56, 0xd0, 0x3c, 0x8a, 0x80, 0x07);
// {400E2D4A-1431-4C1A-A748-39CA472CFDB1}
DEFINE_GUID(IID_IDOManager, 0x400E2D4A, 0x1431, 0x4C1A, 0xA7, 0x48, 0x39, 0xCA, 0x47, 0x2C, 0xFD, 0xB1);

template <typename T>
struct ComReleaser
{
    void operator()(T* pointer) const noexcept
    {
        if (pointer)
            pointer->Release();
    }
};

struct BstrDeleter
{
    void operator()(BSTR string) const noexcept
    {
        if (string)
            SysFreeString(string);
    }
};

struct PhObjectDeleter
{
    template <typename T>
    void operator()(T* pointer) const noexcept
    {
        if (pointer)
            PhDereferenceObject(pointer);
    }
};

template <typename T> using unique_com_ptr = std::unique_ptr<T, ComReleaser<T>>;
using unique_bstr = std::unique_ptr<std::remove_pointer_t<BSTR>, BstrDeleter>;
using unique_ph_string = std::unique_ptr<std::remove_pointer_t<PPH_STRING>, PhObjectDeleter>;

class UpdateDoStatusCallback final : public IDODownloadStatusCallback
{
public:
    UpdateDoStatusCallback(const UpdateDoStatusCallback&) = delete;
    UpdateDoStatusCallback(UpdateDoStatusCallback&&) = delete;
    UpdateDoStatusCallback& operator=(const UpdateDoStatusCallback&) = delete;
    UpdateDoStatusCallback& operator=(UpdateDoStatusCallback&&) = delete;

    explicit UpdateDoStatusCallback(
        _In_ PPH_UPDATER_CONTEXT context
        ) noexcept
        : RefCount(1),
          CompletionEvent(nullptr),
          Status{},
          Context(context)
    {
        PhInitializeQueuedLock(&Lock);
        PhQuerySystemTime(&TimeStart);
    }

    ~UpdateDoStatusCallback()
    {
        if (CompletionEvent)
            NtClose(CompletionEvent);
    }

    /**
     * Initializes the callback instance.
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
     * Queries the current download status.
     * \param status A pointer to a structure that receives the status.
     */
    VOID QueryStatus(
        _Out_ DO_DOWNLOAD_STATUS* status
        ) const noexcept
    {
        PhAcquireQueuedLockShared(const_cast<PPH_QUEUED_LOCK>(&Lock));
        *status = Status;
        PhReleaseQueuedLockShared(const_cast<PPH_QUEUED_LOCK>(&Lock));
    }

    /**
     * \brief Gets the completion event handle.
     * \return The handle to the completion event.
     */
    HANDLE GetCompletionEvent() const noexcept
    {
        return CompletionEvent;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(
        _In_ REFIID riid,
        _Outptr_ VOID** object
        ) override
    {
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDODownloadStatusCallback))
        {
            *object = static_cast<IDODownloadStatusCallback*>(this);
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

    HRESULT STDMETHODCALLTYPE OnStatusChange(
        _In_opt_ IDODownload* download,
        _In_ const DO_DOWNLOAD_STATUS* status
        ) override
    {
        LARGE_INTEGER timeNow;
        ULONG64 timeTicks;
        ULONG64 bytesPerSecond;

        PhAcquireQueuedLockExclusive(&Lock);
        Status = *status;
        PhReleaseQueuedLockExclusive(&Lock);

        PhQuerySystemTime(&timeNow);
        timeTicks = (timeNow.QuadPart - TimeStart.QuadPart) / PH_TICKS_PER_SEC;
        bytesPerSecond = timeTicks ? status->BytesTransferred / timeTicks : 0;

        if (status->BytesTotal != 0 &&
            status->BytesTransferred >= status->BytesTotal &&
            status->State != DODownloadState_Transferred &&
            status->State != DODownloadState_Finalized &&
            status->State != DODownloadState_Aborted &&
            !FAILED(status->Error))
        {
            UpdateSetProgressFinalizingState(Context, L"Finalizing Delivery Optimization download...");
        }
        else
        {
            UpdateSetProgressState(Context, status->BytesTotal, status->BytesTransferred, bytesPerSecond);
        }

        if (status->State == DODownloadState_Transferred ||
            status->State == DODownloadState_Finalized ||
            status->State == DODownloadState_Aborted ||
            FAILED(status->Error))
        {
            NtSetEvent(CompletionEvent, nullptr);
        }

        return S_OK;
    }

private:
    volatile LONG RefCount;
    PH_QUEUED_LOCK Lock;
    HANDLE CompletionEvent;
    LARGE_INTEGER TimeStart;
    DO_DOWNLOAD_STATUS Status;
    PPH_UPDATER_CONTEXT Context;
};

/**
 * Downloads a file using Delivery Optimization.
 *
 * \param Context The updater context.
 * \param Result A pointer to a structure that receives the download results.
 * \return NTSTATUS Successful or errant status.
 */
EXTERN_C NTSTATUS UpdateDownloadFileWithDeliveryOptimization(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PUPDATER_DOWNLOAD_RESULT Result
    )
{
    NTSTATUS status;
    NTSTATUS waitStatus;
    HRESULT result;
    NTSTATUS callbackStatus;
    VARIANT value;
    unique_com_ptr<IClassFactory> idoManagerClassFactory;
    UpdateDoStatusCallback* callback = nullptr;
    unique_com_ptr<IDOManager> idoManagerClass;
    unique_com_ptr<IDODownload> idoDownloadClass;
    unique_ph_string downloadHostPath;
    unique_ph_string downloadUrlPath;
    PPH_STRING rawDownloadHostPath = nullptr;
    PPH_STRING rawDownloadUrlPath = nullptr;
    USHORT downloadPort;
    LARGE_INTEGER timeout;
    DO_DOWNLOAD_STATUS downloadStatus = {};
    unique_ph_string displayName;
    unique_bstr uriString;
    unique_bstr localPathString;
    unique_bstr displayNameString;
    IClassFactory* rawManagerClassFactory = nullptr;
    IDOManager* rawManager = nullptr;
    IDODownload* rawDownload = nullptr;

    if (Context->DialogHandle)
        SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing Delivery Optimization...");
    else if (Context->ToastMode)
        UpdaterUpdateProgressToast(Context, L"Initializing Delivery Optimization...");

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
        CLSID_IDOManager,
        CLSCTX_LOCAL_SERVER,
        nullptr,
        IID_PPV_ARGS(&rawManagerClassFactory)
        );

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    idoManagerClassFactory.reset(rawManagerClassFactory);

    callback = new UpdateDoStatusCallback(Context);

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

    result = idoManagerClassFactory->CreateInstance(
        nullptr,
        IID_PPV_ARGS(&rawManager)
        );

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    idoManagerClass.reset(rawManager);

    result = idoManagerClass->CreateDownload(&rawDownload);

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    idoDownloadClass.reset(rawDownload);

    {
        IClientSecurity* rawClientSecurity = nullptr;
        unique_com_ptr<IClientSecurity> clientSecurityClass;

        result = idoDownloadClass->QueryInterface(
            IID_PPV_ARGS(&rawClientSecurity)
            );

        if (HR_SUCCESS(result))
        {
            clientSecurityClass.reset(rawClientSecurity);

            result = clientSecurityClass->SetBlanket(
                idoDownloadClass.get(),
                RPC_C_AUTHN_WINNT,
                RPC_C_AUTHZ_NONE,
                nullptr,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                nullptr,
                EOAC_NONE
                );

            if (HR_FAILED(result))
            {
                status = UpdateNtStatusFromHresult(result);
                goto CleanupExit;
            }
        }
    }

    uriString.reset(SysAllocString(Context->SetupFileDownloadUrl->Buffer));
    localPathString.reset(SysAllocString(Context->SetupFilePath->Buffer));
    displayName.reset(PhGetBaseName(Context->SetupFilePath));
    displayNameString.reset(SysAllocString(displayName->Buffer));

    if (!uriString || !localPathString || !displayNameString)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    VariantInit(&value);
    value.vt = VT_BSTR;
    value.bstrVal = uriString.get();
    result = idoDownloadClass->SetProperty(DODownloadProperty_Uri, &value);
    value.vt = VT_EMPTY;
    value.bstrVal = nullptr;

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    VariantInit(&value);
    value.vt = VT_BSTR;
    value.bstrVal = localPathString.get();
    result = idoDownloadClass->SetProperty(DODownloadProperty_LocalPath, &value);
    value.vt = VT_EMPTY;
    value.bstrVal = nullptr;

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    VariantInit(&value);
    value.vt = VT_BSTR;
    value.bstrVal = displayNameString.get();
    result = idoDownloadClass->SetProperty(DODownloadProperty_DisplayName, &value);
    value.vt = VT_EMPTY;
    value.bstrVal = nullptr;

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    VariantInit(&value);
    value.vt = VT_BOOL;
    value.boolVal = VARIANT_TRUE;
    result = idoDownloadClass->SetProperty(DODownloadProperty_ForegroundPriority, &value);
    VariantClear(&value);

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    VariantInit(&value);
    value.vt = VT_UNKNOWN;
    value.punkVal = static_cast<IUnknown*>(callback);
    result = idoDownloadClass->SetProperty(DODownloadProperty_CallbackInterface, &value);
    value.vt = VT_EMPTY;
    value.punkVal = nullptr;

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    if (Context->DialogHandle)
    {
        SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, reinterpret_cast<LPARAM>(L"Starting Delivery Optimization download..."));
        SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, reinterpret_cast<LPARAM>(L"Downloaded: ~ of ~ (0%)\r\nSpeed: ~ KB/s"));
    }
    else if (Context->ToastMode)
    {
        UpdaterUpdateProgressToast(Context, L"Starting Delivery Optimization download...");
    }

    result = idoDownloadClass->Start(nullptr);

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    while (TRUE)
    {
        if (Context->Cancel)
        {
            idoDownloadClass->Abort();
            status = STATUS_CANCELLED;
            goto CleanupExit;
        }

        waitStatus = NtWaitForSingleObject(
            callback->GetCompletionEvent(),
            FALSE,
            PhTimeoutFromMilliseconds(&timeout, 200)
            );

        if (waitStatus == STATUS_TIMEOUT)
            continue;

        if (!NT_SUCCESS(waitStatus))
        {
            status = waitStatus;
            goto CleanupExit;
        }

        if (waitStatus == STATUS_WAIT_0)
            break;
    }

    callback->QueryStatus(&downloadStatus);

    if (downloadStatus.State == DODownloadState_Aborted)
    {
        status = HR_FAILED(downloadStatus.Error) ? UpdateNtStatusFromHresult(downloadStatus.Error) : STATUS_CANCELLED;
        goto CleanupExit;
    }

    if (HR_FAILED(downloadStatus.Error))
    {
        status = UpdateNtStatusFromHresult(downloadStatus.Error);
        goto CleanupExit;
    }

    result = idoDownloadClass->Finalize();

    if (HR_FAILED(result))
    {
        status = UpdateNtStatusFromHresult(result);
        goto CleanupExit;
    }

    status = UpdateVerifyCacheFileSignature(Context, Result);

CleanupExit:

    if (callback)
        callback->Release();

    return status;
}

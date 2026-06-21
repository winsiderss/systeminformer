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

//
// This file contains the implementation for checking and applying updates when
// System Informer is packaged for the Windows Store as an MSIX application.
// 
// The updater prefers Windows.Services.Store.StoreContext (Store-acquired
// installs) and falls back to Windows.ApplicationModel.Package update queries
// for the sideloaded .appinstaller distribution. The classic HTTP/WinHTTP and
// ShellExecute pipeline is disabled using an ifdef for these builds.
//

#include "updater.h"

#include <roapi.h>
#include <asyncinfo.h>
#include <wrl.h>
#include <windows.services.store.h>
#include <windows.applicationmodel.h>

using Microsoft::WRL::ComPtr;

// Single-flight stash of the pending Store update set between the check and the
// installation step. The updater shows one modal flow at a time. (dmex)
ComPtr<__FIVectorView_1_Windows__CServices__CStore__CStorePackageUpdate> UpdaterMsixStoreUpdates;

/**
 * \brief Blocks the calling (worker) thread until a Windows Runtime asynchronous
 * operation completes, fails, or is cancelled.
 *
 * \param Context The updater context (for cancellation), or NULL.
 * \param AsyncOperation The IInspectable-derived async operation pointer.
 * \return HRESULT Successful or errant status.
 */
HRESULT UpdaterMsixAwait(
    _In_opt_ PPH_UPDATER_CONTEXT Context,
    _In_ PVOID AsyncOperation
    )
{
    HRESULT status;
    ComPtr<IAsyncInfo> asyncInfo;
    BOOLEAN requestedCancel = FALSE;

    status = static_cast<IUnknown*>(AsyncOperation)->QueryInterface(IID_PPV_ARGS(&asyncInfo));

    if (HR_FAILED(status))
        return status;

    while (TRUE)
    {
        AsyncStatus asyncStatus = Started;

        status = asyncInfo->get_Status(&asyncStatus);

        if (HR_FAILED(status))
            break;

        if (asyncStatus != Started)
        {
            if (asyncStatus == Completed)
            {
                status = S_OK;
            }
            else if (asyncStatus == Canceled)
            {
                status = HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
            else // Error
            {
                HRESULT errorCode = E_FAIL;

                asyncInfo->get_ErrorCode(&errorCode);
                status = HR_FAILED(errorCode) ? errorCode : E_FAIL;
            }

            break;
        }

        if (Context && Context->Cancel && !requestedCancel)
        {
            requestedCancel = TRUE;
            asyncInfo->Cancel();
        }

        PhDelayExecution(100);
    }

    return status;
}

/**
 * \brief Clears the stashed Store update set.
 */
VOID UpdaterMsixClearStoreUpdates(
    VOID
    )
{
    UpdaterMsixStoreUpdates.Reset();
}

/**
 * \brief Converts HRESULT failures to NTSTATUS without relying on exported phlib helpers.
 */
NTSTATUS UpdaterMsixNtStatusFromHResult(
    _In_ HRESULT Result
    )
{
    if (HRESULT_CUSTOMER(Result))
    {
        NOTHING;
    }
    else if (HRESULT_NTSTATUS(Result)) // if (FlagOn(Result, FACILITY_NT_BIT))
    {
        ClearFlag(Result, FACILITY_NT_BIT); // reverse HRESULT_FROM_NT (dmex)
    }
    else if (
        HRESULT_FACILITY(Result) == FACILITY_WIN32 ||
        HRESULT_FACILITY(Result) == FACILITY_WINDOWS
        )
    {
        Result = PhDosErrorToNtStatus(HRESULT_CODE(Result));
    }

    return Result;
}

/**
 * \brief Queries the Microsoft Store for pending package updates for this app.
 *
 * \param Context The updater context.
 * \param UpdateAvailable A pointer to a boolean that receives TRUE if an update is available.
 * \return HRESULT Successful or errant status.
 */
HRESULT UpdaterMsixCheckStoreUpdates(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PBOOLEAN UpdateAvailable
    )
{
    HRESULT status;
    ComPtr<__x_ABI_CWindows_CServices_CStore_CIStoreContextStatics> storeContextStatics;
    ComPtr<__x_ABI_CWindows_CServices_CStore_CIStoreContext> storeContext;
    ComPtr<__FIAsyncOperation_1___FIVectorView_1_Windows__CServices__CStore__CStorePackageUpdate> asyncOperation;
    ComPtr<__FIVectorView_1_Windows__CServices__CStore__CStorePackageUpdate> updates;
    UINT32 updateCount = 0;

    *UpdateAvailable = FALSE;

    status = PhGetActivationFactory(
        L"Windows.Services.Store.dll",
        RuntimeClass_Windows_Services_Store_StoreContext,
        ABI::Windows::Services::Store::IID_IStoreContextStatics,
        reinterpret_cast<PVOID*>(storeContextStatics.GetAddressOf())
        );

    if (HR_FAILED(status))
        return status;

    status = storeContextStatics->GetDefault(storeContext.GetAddressOf());

    if (HR_FAILED(status))
        goto CleanupExit;

    status = storeContext->GetAppAndOptionalStorePackageUpdatesAsync(asyncOperation.GetAddressOf());

    if (HR_FAILED(status))
        goto CleanupExit;

    status = UpdaterMsixAwait(Context, asyncOperation.Get());

    if (HR_FAILED(status))
        goto CleanupExit;

    status = asyncOperation->GetResults(updates.GetAddressOf());

    if (HR_FAILED(status))
        goto CleanupExit;

    status = updates->get_Size(&updateCount);

    if (HR_FAILED(status))
        goto CleanupExit;

    if (updateCount > 0)
    {
        *UpdateAvailable = TRUE;
        UpdaterMsixClearStoreUpdates();
        UpdaterMsixStoreUpdates.Attach(updates.Detach());
    }

CleanupExit:
    return status;
}

/**
 * \brief Queries the platform for an AppInstaller (sideload) update for the current package.
 *
 * \param Context The updater context.
 * \param UpdateAvailable A pointer to a boolean that receives TRUE if an update is available.
 * \return HRESULT Successful or errant status.
 */
static HRESULT UpdaterMsixCheckPackageUpdate(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PBOOLEAN UpdateAvailable
    )
{
    HRESULT status;
    ComPtr<__x_ABI_CWindows_CApplicationModel_CIPackageStatics> packageStatics;
    ComPtr<__x_ABI_CWindows_CApplicationModel_CIPackage> package;
    ComPtr<__x_ABI_CWindows_CApplicationModel_CIPackage6> package6;
    ComPtr<__FIAsyncOperation_1_Windows__CApplicationModel__CPackageUpdateAvailabilityResult> asyncOperation;
    ComPtr<__x_ABI_CWindows_CApplicationModel_CIPackageUpdateAvailabilityResult> result;
    ABI::Windows::ApplicationModel::PackageUpdateAvailability availability = ABI::Windows::ApplicationModel::PackageUpdateAvailability::PackageUpdateAvailability_Unknown;

    *UpdateAvailable = FALSE;

    status = PhGetActivationFactory(
        L"Windows.ApplicationModel.dll",
        RuntimeClass_Windows_ApplicationModel_Package,
        __uuidof(ABI::Windows::ApplicationModel::IPackageStatics),
        reinterpret_cast<PVOID*>(packageStatics.GetAddressOf())
        );

    if (HR_FAILED(status))
        return status;

    status = packageStatics->get_Current(package.GetAddressOf());

    if (HR_FAILED(status))
        goto CleanupExit;

    status = package->QueryInterface(IID_PPV_ARGS(&package6));

    if (HR_FAILED(status))
        goto CleanupExit;

    status = package6->CheckUpdateAvailabilityAsync(asyncOperation.GetAddressOf());

    if (HR_FAILED(status))
        goto CleanupExit;

    status = UpdaterMsixAwait(Context, asyncOperation.Get());

    if (HR_FAILED(status))
        goto CleanupExit;

    status = asyncOperation->GetResults(result.GetAddressOf());

    if (HR_FAILED(status))
        goto CleanupExit;

    status = result->get_Availability(&availability);

    if (HR_FAILED(status))
        goto CleanupExit;

    if (
        availability == ABI::Windows::ApplicationModel::PackageUpdateAvailability::PackageUpdateAvailability_Available ||
        availability == ABI::Windows::ApplicationModel::PackageUpdateAvailability::PackageUpdateAvailability_Required
        )
    {
        *UpdateAvailable = TRUE;
    }

CleanupExit:
    return status;
}

/**
 * \brief Checks for MSIX package updates.
 * \param Context The updater context.
 * \param UpdateAvailable A pointer to a boolean that receives TRUE if an update is available.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS UpdaterMsixCheckForUpdates(
    _In_ PPH_UPDATER_CONTEXT Context,
    _Out_ PBOOLEAN UpdateAvailable
    )
{
    HRESULT status;
    BOOLEAN available = FALSE;

    *UpdateAvailable = FALSE;

    // Prefer the Store servicing path. If the package isn't Store-registered
    // (factory missing, no license, etc.) fall back to the AppInstaller query.
    status = UpdaterMsixCheckStoreUpdates(Context, &available);

    if (HR_FAILED(status))
    {
        status = UpdaterMsixCheckPackageUpdate(Context, &available);
    }

    if (HR_FAILED(status))
        return UpdaterMsixNtStatusFromHResult(status);

    *UpdateAvailable = available;
    return STATUS_SUCCESS;
}

/**
 * \brief Downloads and installs MSIX package updates.
 * \param Context The updater context.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS UpdaterMsixDownloadAndInstall(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    HRESULT status;
    ComPtr<ABI::Windows::Services::Store::IStoreContextStatics> storeContextStatics;
    ComPtr<ABI::Windows::Services::Store::IStoreContext> storeContext;
    ComPtr<IInitializeWithWindow> initializeWithWindow;
    ComPtr<__FIIterable_1_Windows__CServices__CStore__CStorePackageUpdate> updatesIterable;
    ComPtr<__FIAsyncOperationWithProgress_2_Windows__CServices__CStore__CStorePackageUpdateResult_Windows__CServices__CStore__CStorePackageUpdateStatus> asyncOperation;
    ComPtr<ABI::Windows::Services::Store::IStorePackageUpdateResult> result;
    ABI::Windows::Services::Store::StorePackageUpdateState updateState = ABI::Windows::Services::Store::StorePackageUpdateState::StorePackageUpdateState_Pending;

    // No stashed Store update set means we came through the AppInstaller path;
    // Windows applies the .appinstaller update automatically on next launch.
    if (!UpdaterMsixStoreUpdates)
        return STATUS_SUCCESS;

    status = PhGetActivationFactory(
        L"Windows.Services.Store.dll",
        RuntimeClass_Windows_Services_Store_StoreContext,
        __uuidof(ABI::Windows::Services::Store::IStoreContextStatics),
        reinterpret_cast<PVOID*>(storeContextStatics.GetAddressOf())
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = storeContextStatics->GetDefault(storeContext.GetAddressOf());

    if (HR_FAILED(status))
        goto CleanupExit;

    // Desktop apps must associate the StoreContext with a window before any
    // request that may show platform UI.
    if (HR_SUCCESS(storeContext->QueryInterface(IID_PPV_ARGS(&initializeWithWindow))))
    {
        initializeWithWindow->Initialize(SystemInformer_GetWindowHandle());
    }

    // RequestDownloadAndInstallStorePackageUpdatesAsync takes an IIterable.
    status = UpdaterMsixStoreUpdates->QueryInterface(IID_PPV_ARGS(&updatesIterable));

    if (HR_FAILED(status))
        goto CleanupExit;

    status = storeContext->RequestDownloadAndInstallStorePackageUpdatesAsync(
        updatesIterable.Get(),
        asyncOperation.GetAddressOf()
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = UpdaterMsixAwait(Context, asyncOperation.Get());

    if (HR_FAILED(status))
        goto CleanupExit;

    status = asyncOperation->GetResults(result.GetAddressOf());

    if (HR_FAILED(status))
        goto CleanupExit;

    status = result->get_OverallState(&updateState);

    if (HR_FAILED(status))
        goto CleanupExit;

    if (updateState != ABI::Windows::Services::Store::StorePackageUpdateState::StorePackageUpdateState_Completed)
        status = E_FAIL;

CleanupExit:
    UpdaterMsixClearStoreUpdates();

    if (HR_FAILED(status))
        return UpdaterMsixNtStatusFromHResult(status);

    return STATUS_SUCCESS;
}

/**
 * \brief Thread routine for MSIX package updates.
 * \param Parameter The updater context.
 * \return NTSTATUS Successful or errant status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UpdateMsixDownloadThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context = static_cast<PPH_UPDATER_CONTEXT>(Parameter);
    NTSTATUS status;
    //HRESULT roStatus;

    //roStatus = RoInitialize(RO_INIT_MULTITHREADED);

    status = UpdaterMsixDownloadAndInstall(context);

    context->UpdateStatus = status;

    if (NT_SUCCESS(status))
        PostMessage(context->DialogHandle, PH_SHOWINSTALL, 0, 0);
    else
        PostMessage(context->DialogHandle, PH_SHOWERROR, FALSE, FALSE);

    //if (HR_SUCCESS(roStatus))
    //    RoUninitialize();

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}


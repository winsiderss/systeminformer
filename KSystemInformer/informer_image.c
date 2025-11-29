/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *
 */

#include <kph.h>
#include <informer.h>
#include <comms.h>
#include <kphmsgdyn.h>

#include <trace.h>

KPH_PROTECTED_DATA_SECTION_PUSH();
static PVOID KphpImageVerificationCallbackHandle = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

KPH_PAGED_FILE();

/**
 * \brief Performs image tracking.
 *
 * \param[in,out] Process The process loading the image.
 * \param[in] ImageInfo The image info from the notification routine.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpPerformImageTracking(
    _Inout_ PKPH_PROCESS_CONTEXT Process,
    _In_ PIMAGE_INFO_EX ImageInfo
    )
{
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(ImageInfo);

    InterlockedIncrementSizeT(&Process->NumberOfImageLoads);
}

/**
 * \brief Informs any clients of image notify routine invocations.
 *
 * \param[in] FullImageName File name of the loading image.
 * \param[in] ProcessId Process ID where the image is being loaded.
 * \param[in] ImageInfo Information about the image being loaded.
 */
_Function_class_(PLOAD_IMAGE_NOTIFY_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLoadImageNotifyInformer(
    _In_opt_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,
    _In_ PIMAGE_INFO_EX ImageInfo
    )
{
    NTSTATUS status;
    PKPH_PROCESS_CONTEXT targetProcess;
    PKPH_PROCESS_CONTEXT actorProcess;
    PKPH_MESSAGE msg;
    PUNICODE_STRING fileName;
    BOOLEAN freeFileName;

    KPH_PAGED_CODE_PASSIVE();

    actorProcess = KphGetCurrentProcessContext();
    targetProcess = KphGetProcessContext(ProcessId);

    if (!KphInformerEnabled2(ImageLoad, actorProcess, targetProcess))
    {
        goto Exit;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate message");
        goto Exit;
    }

    freeFileName = TRUE;
    status = KphGetNameFileObject(ImageInfo->FileObject, &fileName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphGetNameFileObject failed: %!STATUS!",
                      status);

        freeFileName = FALSE;
        fileName = FullImageName;
    }

    KphMsgInit(msg, KphMsgImageLoad);

    msg->Kernel.ImageLoad.LoadingClientId.UniqueProcess = PsGetCurrentProcessId();
    msg->Kernel.ImageLoad.LoadingClientId.UniqueThread = PsGetCurrentThreadId();
    msg->Kernel.ImageLoad.LoadingProcessStartKey = KphGetCurrentProcessStartKey();
    msg->Kernel.ImageLoad.LoadingThreadSubProcessTag = KphGetCurrentThreadSubProcessTag();
    msg->Kernel.ImageLoad.TargetProcessId = ProcessId;
    msg->Kernel.ImageLoad.Properties = ImageInfo->ImageInfo.Properties;
    msg->Kernel.ImageLoad.ImageBase = ImageInfo->ImageInfo.ImageBase;
    msg->Kernel.ImageLoad.ImageSelector = ImageInfo->ImageInfo.ImageSelector;
    msg->Kernel.ImageLoad.ImageSectionNumber = ImageInfo->ImageInfo.ImageSectionNumber;
    msg->Kernel.ImageLoad.FileObject = ImageInfo->FileObject;

    if (targetProcess)
    {
        ULONG64 startKey;

        startKey = KphGetProcessStartKey(targetProcess->EProcess);

        msg->Kernel.ImageLoad.TargetProcessStartKey = startKey;
    }

    if (fileName)
    {
        status = KphMsgDynAddUnicodeString(msg, KphMsgFieldFileName, fileName);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphMsgDynAddUnicodeString failed: %!STATUS!",
                          status);
        }

        if (freeFileName)
        {
            KphFreeNameFileObject(fileName);
        }
    }

    if (KphInformerOpts2(actorProcess, targetProcess).EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);

Exit:

    if (targetProcess)
    {
        KphDereferenceObject(targetProcess);
    }

    if (actorProcess)
    {
        KphDereferenceObject(actorProcess);
    }
}

/**
 * \brief Image load notify routine.
 *
 * \param[in] FullImageName File name of the loading image.
 * \param[in] ProcessId Process ID where the image is being loaded.
 * \param[in] ImageInfo Information about the image being loaded.
 */
_Function_class_(PLOAD_IMAGE_NOTIFY_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLoadImageNotifyRoutine(
    _In_opt_ PUNICODE_STRING FullImageName,
    _In_ HANDLE ProcessId,
    _In_ PIMAGE_INFO ImageInfo
    )
{
    PKPH_PROCESS_CONTEXT process;
    PIMAGE_INFO_EX imageInfo;

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(FullImageName);
    NT_ASSERT(ImageInfo->ExtendedInfoPresent);

    imageInfo = CONTAINING_RECORD(ImageInfo, IMAGE_INFO_EX, ImageInfo);

    process = KphGetProcessContext(ProcessId);
    if (process)
    {
        KphpPerformImageTracking(process, imageInfo);

        KphApplyImageProtections(process, imageInfo);

        KphDereferenceObject(process);
    }

    KphpLoadImageNotifyInformer(FullImageName, ProcessId, imageInfo);
}

/**
 * \brief Image verification callback.
 *
 * \param[in] CallbackContext Unused.
 * \param[in] ImageType The type of image being verified.
 * \param[in,out] ImageInformation Information about the image being verified.
 */
_IRQL_requires_same_
_Function_class_(SE_IMAGE_VERIFICATION_CALLBACK_FUNCTION)
VOID
KphpImageVerificationCallback(
    _In_opt_ PVOID CallbackContext,
    _In_ SE_IMAGE_TYPE ImageType,
    _Inout_ PBDCB_IMAGE_INFORMATION ImageInformation
    )
{
    NTSTATUS status;
    PKPH_PROCESS_CONTEXT actorProcess;
    PKPH_MESSAGE msg;
    KPHM_SIZED_BUFFER buffer;

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(CallbackContext);

    actorProcess = KphGetCurrentProcessContext();

    if (!KphInformerEnabled(ImageVerify, actorProcess))
    {
        goto Exit;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate message");
        goto Exit;
    }

    KphMsgInit(msg, KphMsgImageVerify);

    msg->Kernel.ImageVerify.ClientId.UniqueProcess = PsGetCurrentProcessId();
    msg->Kernel.ImageVerify.ClientId.UniqueThread = PsGetCurrentThreadId();
    msg->Kernel.ImageVerify.ProcessStartKey = KphGetCurrentProcessStartKey();
    msg->Kernel.ImageVerify.ThreadSubProcessTag = KphGetCurrentThreadSubProcessTag();
    msg->Kernel.ImageVerify.ImageType = ImageType;
    msg->Kernel.ImageVerify.Classification = ImageInformation->Classification;
    msg->Kernel.ImageVerify.ImageFlags = ImageInformation->ImageFlags;
    msg->Kernel.ImageVerify.ImageHashAlgorithm = ImageInformation->ImageHashAlgorithm;
    msg->Kernel.ImageVerify.ThumbprintHashAlgorithm = ImageInformation->ThumbprintHashAlgorithm;

    status = KphMsgDynAddUnicodeString(msg,
                                       KphMsgFieldFileName,
                                       &ImageInformation->ImageName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

    buffer.Buffer = ImageInformation->ImageHash;
    buffer.Size = (USHORT)ImageInformation->ImageHashLength;

    status = KphMsgDynAddSizedBuffer(msg, KphMsgFieldHash, &buffer);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddSizedBuffer failed: %!STATUS!",
                      status);
    }

    status = KphMsgDynAddUnicodeString(msg,
                                       KphMsgFieldCertificatePublisher,
                                       &ImageInformation->CertificatePublisher);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

    status = KphMsgDynAddUnicodeString(msg,
                                       KphMsgFieldCertificateIssuer,
                                       &ImageInformation->CertificateIssuer);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

    buffer.Buffer = ImageInformation->CertificateThumbprint;
    buffer.Size = (USHORT)ImageInformation->CertificateThumbprintLength;

    status = KphMsgDynAddSizedBuffer(msg,
                                     KphMsgFieldCertificateThumbprint,
                                     &buffer);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddSizedBuffer failed: %!STATUS!",
                      status);
    }

    status = KphMsgDynAddUnicodeString(msg,
                                       KphMsgFieldRegistryPath,
                                       &ImageInformation->RegistryPath);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

    if (KphInformerOpts(actorProcess).EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);

Exit:

    if (actorProcess)
    {
        KphDereferenceObject(actorProcess);
    }
}

/**
 * \brief Starts the image informer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphImageInformerStart(
    VOID
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    if (KphDynPsSetLoadImageNotifyRoutineEx)
    {
        status = KphDynPsSetLoadImageNotifyRoutineEx(
                                      KphpLoadImageNotifyRoutine,
                                      PS_IMAGE_NOTIFY_CONFLICTING_ARCHITECTURE);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "Failed to register image notify routine: %!STATUS!",
                          status);

            return status;
        }
    }
    else
    {
        status = PsSetLoadImageNotifyRoutine(KphpLoadImageNotifyRoutine);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "Failed to register image notify routine: %!STATUS!",
                          status);

            return status;
        }
    }

    if (!KphSeRegisterImageVerificationCallback ||
        !KphSeUnregisterImageVerificationCallback)
    {
        return STATUS_SUCCESS;
    }

    status = KphSeRegisterImageVerificationCallback(
                                       SeImageTypeDriver,
                                       SeImageVerificationCallbackInformational,
                                       KphpImageVerificationCallback,
                                       NULL,
                                       NULL,
                                       &KphpImageVerificationCallbackHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to register image verification callback: "
                      "%!STATUS!",
                      status);

        KphpImageVerificationCallbackHandle = NULL;
        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Stops the image informer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphImageInformerStop(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (KphpImageVerificationCallbackHandle)
    {
        NT_ASSERT(KphSeUnregisterImageVerificationCallback);

        KphSeUnregisterImageVerificationCallback(KphpImageVerificationCallbackHandle);
    }

    PsRemoveLoadImageNotifyRoutine(KphpLoadImageNotifyRoutine);
}

/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <comms.h>
#include <dyndata.h>
#include <kphmsgdyn.h>

#include <trace.h>

PAGED_FILE();

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
    PAGED_CODE_PASSIVE();

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
    PKPH_MESSAGE msg;
    PUNICODE_STRING fileName;
    BOOLEAN freeFileName;

    PAGED_CODE_PASSIVE();

    if (!KphInformerSettings.ImageLoad)
    {
        return;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");
        return;
    }

    freeFileName = TRUE;
    status = KphGetNameFileObject(ImageInfo->FileObject, &fileName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      INFORMER,
                      "KphGetNameFileObject failed: %!STATUS!",
                      status);

        freeFileName = FALSE;
        fileName = FullImageName;
    }

    KphMsgInit(msg, KphMsgImageLoad);

    msg->Kernel.ImageLoad.LoadingClientId.UniqueProcess = PsGetCurrentProcessId();
    msg->Kernel.ImageLoad.LoadingClientId.UniqueThread = PsGetCurrentThreadId();
    msg->Kernel.ImageLoad.TargetProcessId = ProcessId;
    msg->Kernel.ImageLoad.Properties = ImageInfo->ImageInfo.Properties;
    msg->Kernel.ImageLoad.ImageBase = ImageInfo->ImageInfo.ImageBase;
    msg->Kernel.ImageLoad.ImageSelector = ImageInfo->ImageInfo.ImageSelector;
    msg->Kernel.ImageLoad.ImageSectionNumber = ImageInfo->ImageInfo.ImageSectionNumber;

    if (fileName)
    {
        status = KphMsgDynAddUnicodeString(msg, KphMsgFieldFileName, fileName);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_WARNING,
                          INFORMER,
                          "KphMsgDynAddUnicodeString failed: %!STATUS!",
                          status);
        }

        if (freeFileName)
        {
            KphFreeNameFileObject(fileName);
        }
    }

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
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

    PAGED_CODE_PASSIVE();

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

    PAGED_CODE_PASSIVE();

    if (KphDynPsSetLoadImageNotifyRoutineEx)
    {
        status = KphDynPsSetLoadImageNotifyRoutineEx(
                                    KphpLoadImageNotifyRoutine,
                                    PS_IMAGE_NOTIFY_CONFLICTING_ARCHITECTURE);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          INFORMER,
                          "Failed to register image notify routine: %!STATUS!",
                          status);
            return status;
        }

        return STATUS_SUCCESS;
    }

    status = PsSetLoadImageNotifyRoutine(KphpLoadImageNotifyRoutine);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to register image notify routine: %!STATUS!",
                      status);
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
    PAGED_CODE_PASSIVE();

    PsRemoveLoadImageNotifyRoutine(KphpLoadImageNotifyRoutine);
}

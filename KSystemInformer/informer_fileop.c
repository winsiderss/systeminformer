/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023-2024
 *
 */

#include <kph.h>
#include <informer.h>
#include <comms.h>
#include <kphmsgdyn.h>
#include <informer_filep.h>

#include <trace.h>

typedef union _KPH_FLT_OPTIONS
{
    USHORT Flags;
    struct
    {
        USHORT PreEnabled : 1;
        USHORT PostEnabled : 1;
        USHORT EnableStackTraces : 1;
        USHORT EnablePostFileNames : 1;
        USHORT EnablePagingIo : 1;
        USHORT EnableSyncPagingIo : 1;
        USHORT EnableIoControlBuffers : 1;
        USHORT EnableFsControlBuffers : 1;
        USHORT EnableDirControlBuffers : 1;
        USHORT EnablePreCreateReply : 1;
        USHORT EnablePostCreateReply : 1;
        USHORT Spare : 5;
    };
} KPH_FLT_OPTIONS, *PKPH_FLT_OPTIONS;

typedef struct _KPH_FLT_COMPLETION_CONTEXT
{
    PKPH_MESSAGE Message;
    KPH_FLT_OPTIONS Options;
    PFLT_FILE_NAME_INFORMATION FileNameInfo;
    PFLT_FILE_NAME_INFORMATION DestFileNameInfo;
} KPH_FLT_COMPLETION_CONTEXT, *PKPH_FLT_COMPLETION_CONTEXT;

static volatile ULONG64 KphpFltSequence = 0;
static BOOLEAN KphpFltOpInitialized = FALSE;
static NPAGED_LOOKASIDE_LIST KphpFltCompletionContextLookaside = { 0 };

/**
 * \brief Gets the options for the filter.
 *
 * \param[in] Data The callback data for the operation.
 *
 * \return Options for the filter.
 */
_IRQL_requires_max_(APC_LEVEL)
KPH_FLT_OPTIONS KphpFltGetOptions(
    _In_ PFLT_CALLBACK_DATA Data
    )
{
    KPH_FLT_OPTIONS options;
    PKPH_PROCESS_CONTEXT process;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    options.Flags = 0;

    if (Data->Thread)
    {
        process = KphGetEProcessContext(PsGetThreadProcess(Data->Thread));
    }
    else
    {
        process = KphGetSystemProcessContext();
    }

#define KPH_FLT_SETTING(majorFunction, name)                                  \
    case majorFunction:                                                       \
    {                                                                         \
        if (KphInformerEnabled(FilePre##name, process))                       \
        {                                                                     \
            options.PreEnabled = TRUE;                                        \
        }                                                                     \
        if (KphInformerEnabled(FilePost##name, process))                      \
        {                                                                     \
            options.PostEnabled = TRUE;                                       \
        }                                                                     \
        break;                                                                \
    }

    switch (Data->Iopb->MajorFunction)
    {
        KPH_FLT_SETTING(IRP_MJ_CREATE, Create)
        KPH_FLT_SETTING(IRP_MJ_CREATE_NAMED_PIPE, CreateNamedPipe)
        KPH_FLT_SETTING(IRP_MJ_CLOSE, Close)
        KPH_FLT_SETTING(IRP_MJ_READ, Read)
        KPH_FLT_SETTING(IRP_MJ_WRITE, Write)
        KPH_FLT_SETTING(IRP_MJ_QUERY_INFORMATION, QueryInformation)
        KPH_FLT_SETTING(IRP_MJ_SET_INFORMATION, SetInformation)
        KPH_FLT_SETTING(IRP_MJ_QUERY_EA, QueryEa)
        KPH_FLT_SETTING(IRP_MJ_SET_EA, SetEa)
        KPH_FLT_SETTING(IRP_MJ_FLUSH_BUFFERS, FlushBuffers)
        KPH_FLT_SETTING(IRP_MJ_QUERY_VOLUME_INFORMATION, QueryVolumeInformation)
        KPH_FLT_SETTING(IRP_MJ_SET_VOLUME_INFORMATION, SetVolumeInformation)
        KPH_FLT_SETTING(IRP_MJ_DIRECTORY_CONTROL, DirectoryControl)
        KPH_FLT_SETTING(IRP_MJ_FILE_SYSTEM_CONTROL, FileSystemControl)
        KPH_FLT_SETTING(IRP_MJ_DEVICE_CONTROL, DeviceControl)
        KPH_FLT_SETTING(IRP_MJ_INTERNAL_DEVICE_CONTROL, InternalDeviceControl)
        KPH_FLT_SETTING(IRP_MJ_SHUTDOWN, Shutdown)
        KPH_FLT_SETTING(IRP_MJ_LOCK_CONTROL, LockControl)
        KPH_FLT_SETTING(IRP_MJ_CLEANUP, Cleanup)
        KPH_FLT_SETTING(IRP_MJ_CREATE_MAILSLOT, CreateMailslot)
        KPH_FLT_SETTING(IRP_MJ_QUERY_SECURITY, QuerySecurity)
        KPH_FLT_SETTING(IRP_MJ_SET_SECURITY, SetSecurity)
        KPH_FLT_SETTING(IRP_MJ_POWER, Power)
        KPH_FLT_SETTING(IRP_MJ_SYSTEM_CONTROL, SystemControl)
        KPH_FLT_SETTING(IRP_MJ_DEVICE_CHANGE, DeviceChange)
        KPH_FLT_SETTING(IRP_MJ_QUERY_QUOTA, QueryQuota)
        KPH_FLT_SETTING(IRP_MJ_SET_QUOTA, SetQuota)
        KPH_FLT_SETTING(IRP_MJ_PNP, Pnp)
        KPH_FLT_SETTING(IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION, AcquireForSectionSync)
        KPH_FLT_SETTING(IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION, ReleaseForSectionSync)
        KPH_FLT_SETTING(IRP_MJ_ACQUIRE_FOR_MOD_WRITE, AcquireForModWrite)
        KPH_FLT_SETTING(IRP_MJ_RELEASE_FOR_MOD_WRITE, ReleaseForModWrite)
        KPH_FLT_SETTING(IRP_MJ_ACQUIRE_FOR_CC_FLUSH, AcquireForCcFlush)
        KPH_FLT_SETTING(IRP_MJ_RELEASE_FOR_CC_FLUSH, ReleaseForCcFlush)
        KPH_FLT_SETTING(IRP_MJ_QUERY_OPEN, QueryOpen)
        KPH_FLT_SETTING(IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE, FastIoCheckIfPossible)
        KPH_FLT_SETTING(IRP_MJ_NETWORK_QUERY_OPEN, NetworkQueryOpen)
        KPH_FLT_SETTING(IRP_MJ_MDL_READ, MdlRead)
        KPH_FLT_SETTING(IRP_MJ_MDL_READ_COMPLETE, MdlReadComplete)
        KPH_FLT_SETTING(IRP_MJ_PREPARE_MDL_WRITE, PrepareMdlWrite)
        KPH_FLT_SETTING(IRP_MJ_MDL_WRITE_COMPLETE, MdlWriteComplete)
        KPH_FLT_SETTING(IRP_MJ_VOLUME_MOUNT, VolumeMount)
        KPH_FLT_SETTING(IRP_MJ_VOLUME_DISMOUNT, VolumeDismount)
        DEFAULT_UNREACHABLE;
    }

    if (options.PreEnabled || options.PostEnabled)
    {
        options.EnableStackTraces = KphInformerEnabled(EnableStackTraces, process);
        options.EnablePostFileNames = KphInformerEnabled(FileEnablePostFileNames, process);
        options.EnablePagingIo = KphInformerEnabled(FileEnablePagingIo, process);
        options.EnableSyncPagingIo = KphInformerEnabled(FileEnableSyncPagingIo, process);
        options.EnableIoControlBuffers = KphInformerEnabled(FileEnableIoControlBuffers, process);
        options.EnableFsControlBuffers = KphInformerEnabled(FileEnableFsControlBuffers, process);
        options.EnableDirControlBuffers = KphInformerEnabled(FileEnableDirControlBuffers, process);
        options.EnablePreCreateReply = KphInformerEnabled(FileEnablePreCreateReply, process);
        options.EnablePostCreateReply = KphInformerEnabled(FileEnablePostCreateReply, process);
    }

    if (process)
    {
        KphDereferenceObject(process);
    }

    return options;
}

/**
 * \brief Retrieves the message ID for a given operation.
 *
 * \param[in] MajorFunction The major function of the operation.
 * \param[in] PostOperation Denotes if this is a pre or post operation message.
 *
 * \return The message ID for the operation.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
KPH_MESSAGE_ID KphpFltGetMessageId(
    _In_ UCHAR MajorFunction,
    _In_ BOOLEAN PostOperation
    )
{
    KPH_MESSAGE_ID messageId;

    NPAGED_CODE_DISPATCH_MAX();

#define KPH_FLT_MESSAGE_ID(majorFunction, name)                               \
    case majorFunction:                                                       \
    {                                                                         \
        if (PostOperation)                                                    \
        {                                                                     \
            messageId = KphMsgFilePost##name;                                 \
        }                                                                     \
        else                                                                  \
        {                                                                     \
            messageId = KphMsgFilePre##name;                                  \
        }                                                                     \
        break;                                                                \
    }

    switch (MajorFunction)
    {
        KPH_FLT_MESSAGE_ID(IRP_MJ_CREATE, Create)
        KPH_FLT_MESSAGE_ID(IRP_MJ_CREATE_NAMED_PIPE, CreateNamedPipe)
        KPH_FLT_MESSAGE_ID(IRP_MJ_CLOSE, Close)
        KPH_FLT_MESSAGE_ID(IRP_MJ_READ, Read)
        KPH_FLT_MESSAGE_ID(IRP_MJ_WRITE, Write)
        KPH_FLT_MESSAGE_ID(IRP_MJ_QUERY_INFORMATION, QueryInformation)
        KPH_FLT_MESSAGE_ID(IRP_MJ_SET_INFORMATION, SetInformation)
        KPH_FLT_MESSAGE_ID(IRP_MJ_QUERY_EA, QueryEa)
        KPH_FLT_MESSAGE_ID(IRP_MJ_SET_EA, SetEa)
        KPH_FLT_MESSAGE_ID(IRP_MJ_FLUSH_BUFFERS, FlushBuffers)
        KPH_FLT_MESSAGE_ID(IRP_MJ_QUERY_VOLUME_INFORMATION, QueryVolumeInformation)
        KPH_FLT_MESSAGE_ID(IRP_MJ_SET_VOLUME_INFORMATION, SetVolumeInformation)
        KPH_FLT_MESSAGE_ID(IRP_MJ_DIRECTORY_CONTROL, DirectoryControl)
        KPH_FLT_MESSAGE_ID(IRP_MJ_FILE_SYSTEM_CONTROL, FileSystemControl)
        KPH_FLT_MESSAGE_ID(IRP_MJ_DEVICE_CONTROL, DeviceControl)
        KPH_FLT_MESSAGE_ID(IRP_MJ_INTERNAL_DEVICE_CONTROL, InternalDeviceControl)
        KPH_FLT_MESSAGE_ID(IRP_MJ_SHUTDOWN, Shutdown)
        KPH_FLT_MESSAGE_ID(IRP_MJ_LOCK_CONTROL, LockControl)
        KPH_FLT_MESSAGE_ID(IRP_MJ_CLEANUP, Cleanup)
        KPH_FLT_MESSAGE_ID(IRP_MJ_CREATE_MAILSLOT, CreateMailslot)
        KPH_FLT_MESSAGE_ID(IRP_MJ_QUERY_SECURITY, QuerySecurity)
        KPH_FLT_MESSAGE_ID(IRP_MJ_SET_SECURITY, SetSecurity)
        KPH_FLT_MESSAGE_ID(IRP_MJ_POWER, Power)
        KPH_FLT_MESSAGE_ID(IRP_MJ_SYSTEM_CONTROL, SystemControl)
        KPH_FLT_MESSAGE_ID(IRP_MJ_DEVICE_CHANGE, DeviceChange)
        KPH_FLT_MESSAGE_ID(IRP_MJ_QUERY_QUOTA, QueryQuota)
        KPH_FLT_MESSAGE_ID(IRP_MJ_SET_QUOTA, SetQuota)
        KPH_FLT_MESSAGE_ID(IRP_MJ_PNP, Pnp)
        KPH_FLT_MESSAGE_ID(IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION, AcquireForSectionSync)
        KPH_FLT_MESSAGE_ID(IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION, ReleaseForSectionSync)
        KPH_FLT_MESSAGE_ID(IRP_MJ_ACQUIRE_FOR_MOD_WRITE, AcquireForModWrite)
        KPH_FLT_MESSAGE_ID(IRP_MJ_RELEASE_FOR_MOD_WRITE, ReleaseForModWrite)
        KPH_FLT_MESSAGE_ID(IRP_MJ_ACQUIRE_FOR_CC_FLUSH, AcquireForCcFlush)
        KPH_FLT_MESSAGE_ID(IRP_MJ_RELEASE_FOR_CC_FLUSH, ReleaseForCcFlush)
        KPH_FLT_MESSAGE_ID(IRP_MJ_QUERY_OPEN, QueryOpen)
        KPH_FLT_MESSAGE_ID(IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE, FastIoCheckIfPossible)
        KPH_FLT_MESSAGE_ID(IRP_MJ_NETWORK_QUERY_OPEN, NetworkQueryOpen)
        KPH_FLT_MESSAGE_ID(IRP_MJ_MDL_READ, MdlRead)
        KPH_FLT_MESSAGE_ID(IRP_MJ_MDL_READ_COMPLETE, MdlReadComplete)
        KPH_FLT_MESSAGE_ID(IRP_MJ_PREPARE_MDL_WRITE, PrepareMdlWrite)
        KPH_FLT_MESSAGE_ID(IRP_MJ_MDL_WRITE_COMPLETE, MdlWriteComplete)
        KPH_FLT_MESSAGE_ID(IRP_MJ_VOLUME_MOUNT, VolumeMount)
        KPH_FLT_MESSAGE_ID(IRP_MJ_VOLUME_DISMOUNT, VolumeDismount)
        DEFAULT_UNREACHABLE;
    }

    return messageId;
}

/**
 * \brief Initializes a message for a file operation.
 *
 * \param[in,out] Message The message to initialize.
 * \param[in] FltObjects The filter objects for the operation.
 * \param[in] MajorFunction The major function of the operation.
 * \param[in] PostOperation Denotes if this is a pre or post operation message.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpFltInitMessage(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ UCHAR MajorFunction,
    _In_ BOOLEAN PostOperation
    )
{
    ULONG_PTR stackLowLimit;
    ULONG_PTR stackHighLimit;
    POPLOCK_KEY_CONTEXT oplockKeyContext;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    KphMsgInit(Message, KphpFltGetMessageId(MajorFunction, PostOperation));

    Message->Kernel.File.TransactionContext = FltObjects->TransactionContext;
    Message->Kernel.File.Volume = FltObjects->Volume;
    Message->Kernel.File.FileObject = FltObjects->FileObject;
    Message->Kernel.File.Transaction = FltObjects->Transaction;

    if (!FltObjects->FileObject)
    {
        return;
    }

    if (FltObjects->FileObject->SectionObjectPointer)
    {
        PSECTION_OBJECT_POINTERS sectionPointers;

        sectionPointers = FltObjects->FileObject->SectionObjectPointer;

        Message->Kernel.File.DataSectionObject = sectionPointers->DataSectionObject;
        Message->Kernel.File.ImageSectionObject = sectionPointers->ImageSectionObject;
    }

    if (FsRtlIsPagingFile(FltObjects->FileObject))
    {
        Message->Kernel.File.IsPagingFile = TRUE;
    }

    if (FsRtlIsSystemPagingFile(FltObjects->FileObject))
    {
        Message->Kernel.File.IsSystemPagingFile = TRUE;
    }

    if (IoIsFileOriginRemote(FltObjects->FileObject))
    {
        Message->Kernel.File.OriginRemote = TRUE;
    }

    if (IoIsFileObjectIgnoringSharing(FltObjects->FileObject))
    {
        Message->Kernel.File.IgnoringSharing = TRUE;
    }

    IoGetStackLimits(&stackLowLimit, &stackHighLimit);
    if (((ULONG_PTR)FltObjects->FileObject >= stackLowLimit) &&
        ((ULONG_PTR)FltObjects->FileObject <= stackHighLimit))
    {
        Message->Kernel.File.InStackFileObject = TRUE;
    }

    Message->Kernel.File.LockOperation = (FltObjects->FileObject->LockOperation != FALSE);
    Message->Kernel.File.ReadAccess = (FltObjects->FileObject->ReadAccess != FALSE);
    Message->Kernel.File.WriteAccess = (FltObjects->FileObject->WriteAccess != FALSE);
    Message->Kernel.File.DeleteAccess = (FltObjects->FileObject->DeleteAccess != FALSE);
    Message->Kernel.File.SharedRead = (FltObjects->FileObject->SharedRead != FALSE);
    Message->Kernel.File.SharedWrite = (FltObjects->FileObject->SharedWrite != FALSE);
    Message->Kernel.File.SharedDelete = (FltObjects->FileObject->SharedDelete != FALSE);

    oplockKeyContext = IoGetOplockKeyContextEx(FltObjects->FileObject);
    if (oplockKeyContext)
    {
        RtlCopyMemory(&Message->Kernel.File.OplockKeyContext,
                      oplockKeyContext,
                      sizeof(OPLOCK_KEY_CONTEXT));
    }
}

/**
 * \brief Copies a file name into a message.
 *
 * \param[in,out] Message The message to copy the file name into.
 * \param[in] FieldId The field to copy the file name into.
 * \param[in] FltFileName The file name information to copy.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpFltCopyFileName(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PKPH_FLT_FILE_NAME FltFileName
    )
{
    NTSTATUS status;
    PUNICODE_STRING string;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    switch (FltFileName->Type)
    {
        case KphFltFileNameTypeNameInfo:
        {
            string = &FltFileName->NameInfo->Name;
            break;
        }
        case KphFltFileNameTypeContext:
        {
            //
            // N.B. PFLT_CONTEXT stores a PUNICODE_STRING, this is a union so
            // fall through and use the FileName field.
            //
            NT_ASSERT(FltFileName->Context == FltFileName->FileName);
            __fallthrough;
        }
        case KphFltFileNameTypeFileName:
        {
            string = FltFileName->FileName;
            break;
        }
        case KphFltFileNameTypeNameCache:
        {
            string = &FltFileName->NameCache->FileName;
            break;
        }
        default:
        {
            return;
        }
    }

    status = KphMsgDynAddUnicodeString(Message, FieldId, string);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }
}

/**
 * \brief Copies a file system buffer into a message.
 *
 * \details This function handles various buffers from the file system filter
 * to capture and copy information into the message. This function is overall
 * best effort. It can truncate buffers to fit into the message.
 *
 * \param[in,out] Message The message to copy the buffer into.
 * \param[in] Data The callback data for the operation.
 * \param[in] FieldId The field to copy the buffer into. Optional if DestBuffer
 * is non-NULL. Certain Unicode string fields are special cased to be copied as
 * a Unicode string instead of a sized buffer.
 * \param[in] SystemBuffer Denotes if the buffer is a system buffer. If TRUE
 * this function assumes the buffer doesn't need to be probed and locked. In
 * some cases the filter parameters are known to be a system buffer, such as
 * METHOD_BUFFERED.
 * \param[out] DestBuffer Optional destination buffer to copy into, if NULL the
 * data will be copied into the dynamic message buffer for FieldId.
 * \param[in] DestLength The length of the destination buffer.
 * \param[in] Mdl Optional memory descriptor list which the buffer will be
 * copied from instead, if non-NULL the MDL is used instead of any provided
 * input buffer.
 * \param[in] Buffer Optional buffer to copy from, if NULL and the Mdl is NULL
 * this function will do nothing.
 * \param[in] Length The length to copy.
 * \param[in] Truncate If TRUE the buffer will be truncated to fit into the
 * message, otherwise the buffer copy may fail.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpFltCopyBuffer(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PFLT_CALLBACK_DATA Data,
    _In_opt_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ BOOLEAN SystemBuffer,
    _Out_writes_bytes_to_opt_(DestLength, Length) PVOID DestBuffer,
    _In_ ULONG DestLength,
    _In_opt_ PMDL Mdl,
    _In_opt_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN Truncate
    )
{
    NTSTATUS status;
    PVOID buffer;
    PMDL mdl;
    BOOLEAN unlockPages;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    mdl = NULL;
    unlockPages = FALSE;

    if (DestBuffer)
    {
        NT_ASSERT(FieldId == InvalidKphMsgField);
        NT_ASSERT(DestLength >= Length);

        RtlZeroMemory(DestBuffer, DestLength);
    }
    else if (Truncate)
    {
        USHORT remaining;

        NT_ASSERT(FieldId != InvalidKphMsgField);

        remaining = KphMsgDynRemaining(Message);

        if (Length > remaining)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "Buffer too large for message, truncating %lu -> %lu",
                          Length,
                          remaining);

            Length = remaining;
        }
    }

    if (!Length)
    {
        goto Exit;
    }

    if (Mdl)
    {
        buffer = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);
        goto CopyBuffer;
    }

    if (SystemBuffer ||
        FLT_IS_FASTIO_OPERATION(Data) ||
        FLT_IS_SYSTEM_BUFFER(Data))
    {
        buffer = Buffer;
        goto CopyBuffer;
    }

    if (!Buffer)
    {
        goto Exit;
    }

    if (!Data->Thread)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Missing thread for buffer capture");
        goto Exit;
    }

    mdl = IoAllocateMdl(Buffer, Length, FALSE, FALSE, NULL);
    if (!mdl)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate MDL");
        goto Exit;
    }

    __try
    {
        MmProbeAndLockProcessPages(mdl,
                                   PsGetThreadProcess(Data->Thread),
                                   KernelMode,
                                   IoReadAccess);
        unlockPages = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "MmProbeAndLockProcessPages failed: %!STATUS!",
                      GetExceptionCode());
        goto Exit;
    }

    buffer = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);

CopyBuffer:

    if (!buffer)
    {
        goto Exit;
    }

    __try
    {
        if (DestBuffer)
        {
            NT_ASSERT(FieldId == InvalidKphMsgField);
            NT_ASSERT(Length <= DestLength);

            RtlCopyMemory(DestBuffer, buffer, Length);
        }
        else if (FieldId == KphMsgFieldDestinationFileName)
        {
            UNICODE_STRING fileName;

            NT_ASSERT(Length <= USHORT_MAX);

            fileName.Buffer = Buffer;
            fileName.Length = (USHORT)Length;
            fileName.MaximumLength = fileName.Length;

            status = KphMsgDynAddUnicodeString(Message, FieldId, &fileName);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              INFORMER,
                              "KphMsgDynAddSizedBuffer failed: %!STATUS!",
                              status);
            }
        }
        else
        {
            KPHM_SIZED_BUFFER sizedBuffer;

            NT_ASSERT(FieldId != InvalidKphMsgField);
            NT_ASSERT(Length <= USHORT_MAX);

            sizedBuffer.Buffer = buffer;
            sizedBuffer.Size = (USHORT)Length;

            status = KphMsgDynAddSizedBuffer(Message, FieldId, &sizedBuffer);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              INFORMER,
                              "KphMsgDynAddSizedBuffer failed: %!STATUS!",
                              status);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Exception copying buffer: %!STATUS!",
                      GetExceptionCode());

        if (FieldId != InvalidKphMsgField)
        {
            //
            // N.B. The KphMsgDynAdd* routines will first reserve a table
            // entry then copy data into the buffer. If we get here it means
            // the control flow was arrested after the reservation. Since we
            // can't guarantee it was all copied successfully clear the last
            // entry that was reserved.
            //
            KphMsgDynClearLast(Message);
        }
    }

Exit:

    if (mdl)
    {
        if (unlockPages)
        {
            MmUnlockPages(mdl);
        }

        IoFreeMdl(mdl);
    }
}

/**
 * \brief Copies file system control buffers into the message.
 *
 * \param[in,out] Message The message to copy the buffers into.
 * \param[in] Data The callback data for the operation.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpFltCopyFsControl(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PFLT_CALLBACK_DATA Data
    )
{
    UCHAR method;
    PMDL mdl;
    PVOID buffer;
    ULONG length;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    NT_ASSERT(Data->Iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL);

    method = METHOD_FROM_CTL_CODE(Data->Iopb->Parameters.FileSystemControl.Common.FsControlCode);

    switch (method)
    {
        case METHOD_NEITHER:
        {
            //
            // Capture the input and output buffer always since it is possible
            // to use them interchangeably.
            //

            buffer = Data->Iopb->Parameters.FileSystemControl.Neither.InputBuffer;
            if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION))
            {
                length = min((ULONG)Data->IoStatus.Information,
                             Data->Iopb->Parameters.FileSystemControl.Neither.InputBufferLength);
            }
            else
            {
                length = Data->Iopb->Parameters.FileSystemControl.Neither.InputBufferLength;
            }

            KphpFltCopyBuffer(Message,
                              Data,
                              KphMsgFieldInput,
                              FALSE,
                              NULL,
                              0,
                              NULL,
                              buffer,
                              length,
                              TRUE);

            mdl = Data->Iopb->Parameters.FileSystemControl.Neither.OutputMdlAddress;
            buffer = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBuffer;
            if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION))
            {
                length = min((ULONG)Data->IoStatus.Information,
                             Data->Iopb->Parameters.FileSystemControl.Neither.OutputBufferLength);
            }
            else
            {
                length = Data->Iopb->Parameters.FileSystemControl.Neither.OutputBufferLength;
            }

            KphpFltCopyBuffer(Message,
                              Data,
                              KphMsgFieldOutput,
                              FALSE,
                              NULL,
                              0,
                              mdl,
                              buffer,
                              length,
                              TRUE);
            break;
        }
        case METHOD_BUFFERED:
        {
            buffer = Data->Iopb->Parameters.FileSystemControl.Buffered.SystemBuffer;

            if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION))
            {
                length = (ULONG)Data->IoStatus.Information;

                KphpFltCopyBuffer(Message,
                                  Data,
                                  KphMsgFieldOutput,
                                  TRUE,
                                  NULL,
                                  0,
                                  NULL,
                                  buffer,
                                  length,
                                  TRUE);

            }
            else
            {
                length = Data->Iopb->Parameters.FileSystemControl.Buffered.InputBufferLength;

                KphpFltCopyBuffer(Message,
                                  Data,
                                  KphMsgFieldInput,
                                  TRUE,
                                  NULL,
                                  0,
                                  NULL,
                                  buffer,
                                  length,
                                  TRUE);
            }
            break;
        }
        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:
        {
            if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION))
            {
                mdl = Data->Iopb->Parameters.FileSystemControl.Direct.OutputMdlAddress;
                buffer = Data->Iopb->Parameters.FileSystemControl.Direct.OutputBuffer;
                length = (ULONG)Data->IoStatus.Information;

                KphpFltCopyBuffer(Message,
                                  Data,
                                  KphMsgFieldOutput,
                                  FALSE,
                                  NULL,
                                  0,
                                  mdl,
                                  buffer,
                                  length,
                                  TRUE);
            }
            else
            {
                buffer = Data->Iopb->Parameters.FileSystemControl.Direct.InputSystemBuffer;
                length = Data->Iopb->Parameters.FileSystemControl.Direct.InputBufferLength;

                KphpFltCopyBuffer(Message,
                                  Data,
                                  KphMsgFieldInput,
                                  TRUE,
                                  NULL,
                                  0,
                                  NULL,
                                  buffer,
                                  length,
                                  TRUE);
            }
            break;
        }
        default:
        {
            return;
        }
    }
}

/**
 * \brief Copies device control buffers into the message.
 *
 * \param[in,out] Message The message to copy the buffers into.
 * \param[in] Data The callback data for the operation.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpFltCopyIoControl(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PFLT_CALLBACK_DATA Data
    )
{
    UCHAR method;
    PMDL mdl;
    PVOID buffer;
    ULONG length;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    NT_ASSERT((Data->Iopb->MajorFunction == IRP_MJ_DEVICE_CONTROL) ||
              (Data->Iopb->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL));

    if (FLT_IS_FASTIO_OPERATION(Data))
    {
        //
        // Capture the input and output buffer always since it is possible
        // to use them interchangeably.
        //

        buffer = Data->Iopb->Parameters.DeviceIoControl.FastIo.InputBuffer;
        if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION))
        {
            length = min((ULONG)Data->IoStatus.Information,
                         length = Data->Iopb->Parameters.DeviceIoControl.FastIo.InputBufferLength);

        }
        else
        {
            length = Data->Iopb->Parameters.DeviceIoControl.FastIo.InputBufferLength;
        }

        KphpFltCopyBuffer(Message,
                          Data,
                          KphMsgFieldInput,
                          FALSE,
                          NULL,
                          0,
                          NULL,
                          buffer,
                          length,
                          TRUE);

        buffer = Data->Iopb->Parameters.DeviceIoControl.FastIo.OutputBuffer;
        if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION))
        {
            length = min((ULONG)Data->IoStatus.Information,
                         Data->Iopb->Parameters.DeviceIoControl.FastIo.OutputBufferLength);
        }
        else
        {
            length = Data->Iopb->Parameters.DeviceIoControl.FastIo.OutputBufferLength;
        }

        KphpFltCopyBuffer(Message,
                          Data,
                          KphMsgFieldOutput,
                          FALSE,
                          NULL,
                          0,
                          NULL,
                          buffer,
                          length,
                          TRUE);
        return;
    }

    method = METHOD_FROM_CTL_CODE(Data->Iopb->Parameters.DeviceIoControl.Common.IoControlCode);

    switch (method)
    {
        case METHOD_NEITHER:
        {
            //
            // Capture the input and output buffer always since it is possible
            // to use them interchangeably.
            //

            buffer = Data->Iopb->Parameters.DeviceIoControl.Neither.InputBuffer;
            if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION))
            {
                length = min((ULONG)Data->IoStatus.Information,
                             Data->Iopb->Parameters.DeviceIoControl.Neither.InputBufferLength);
            }
            else
            {
                length = Data->Iopb->Parameters.DeviceIoControl.Neither.InputBufferLength;
            }

            KphpFltCopyBuffer(Message,
                              Data,
                              KphMsgFieldInput,
                              FALSE,
                              NULL,
                              0,
                              NULL,
                              buffer,
                              length,
                              TRUE);

            mdl = Data->Iopb->Parameters.DeviceIoControl.Neither.OutputMdlAddress;
            buffer = Data->Iopb->Parameters.DeviceIoControl.Neither.OutputBuffer;
            if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION))
            {
                length = min((ULONG)Data->IoStatus.Information,
                             Data->Iopb->Parameters.DeviceIoControl.Neither.OutputBufferLength);
            }
            else
            {
                length = Data->Iopb->Parameters.DeviceIoControl.Neither.OutputBufferLength;
            }

            KphpFltCopyBuffer(Message,
                              Data,
                              KphMsgFieldOutput,
                              FALSE,
                              NULL,
                              0,
                              mdl,
                              buffer,
                              length,
                              TRUE);
            break;
        }
        case METHOD_BUFFERED:
        {
            buffer = Data->Iopb->Parameters.DeviceIoControl.Buffered.SystemBuffer;

            if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION))
            {
                length = (ULONG)Data->IoStatus.Information;

                KphpFltCopyBuffer(Message,
                                  Data,
                                  KphMsgFieldOutput,
                                  TRUE,
                                  NULL,
                                  0,
                                  NULL,
                                  buffer,
                                  length,
                                  TRUE);

            }
            else
            {
                length = Data->Iopb->Parameters.DeviceIoControl.Buffered.InputBufferLength;

                KphpFltCopyBuffer(Message,
                                  Data,
                                  KphMsgFieldInput,
                                  TRUE,
                                  NULL,
                                  0,
                                  NULL,
                                  buffer,
                                  length,
                                  TRUE);
            }
            break;
        }
        case METHOD_IN_DIRECT:
        case METHOD_OUT_DIRECT:
        {
            if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_POST_OPERATION))
            {
                mdl = Data->Iopb->Parameters.DeviceIoControl.Direct.OutputMdlAddress;
                buffer = Data->Iopb->Parameters.DeviceIoControl.Direct.OutputBuffer;
                length = (ULONG)Data->IoStatus.Information;

                KphpFltCopyBuffer(Message,
                                  Data,
                                  KphMsgFieldOutput,
                                  FALSE,
                                  NULL,
                                  0,
                                  mdl,
                                  buffer,
                                  length,
                                  TRUE);
            }
            else
            {
                buffer = Data->Iopb->Parameters.DeviceIoControl.Direct.InputSystemBuffer;
                length = Data->Iopb->Parameters.DeviceIoControl.Direct.InputBufferLength;

                KphpFltCopyBuffer(Message,
                                  Data,
                                  KphMsgFieldInput,
                                  TRUE,
                                  NULL,
                                  0,
                                  NULL,
                                  buffer,
                                  length,
                                  TRUE);
            }
            break;
        }
        default:
        {
            return;
        }
    }
}

/**
 * \brief Fills a message with common information.
 *
 * \param[in,out] Message The message to fill.
 * \param[in] Data The callback data for the operation.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpFltFillCommonMessage(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PFLT_CALLBACK_DATA Data
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    if (Data->Thread)
    {
        PEPROCESS process;
        BOOLEAN cacheOnly;

        process = PsGetThreadProcess(Data->Thread);

        Message->Kernel.File.ClientId.UniqueProcess = PsGetProcessId(process);
        Message->Kernel.File.ClientId.UniqueThread = PsGetThreadId(Data->Thread);
        Message->Kernel.File.ProcessStartKey = KphGetProcessStartKey(process);

        cacheOnly = (FlagOn(Data->Iopb->IrpFlags, IRP_PAGING_IO) ||
                     FlagOn(Data->Iopb->IrpFlags, IRP_SYNCHRONOUS_PAGING_IO));

        Message->Kernel.File.ThreadSubProcessTag = KphGetThreadSubProcessTagEx(Data->Thread, cacheOnly);
    }

    Message->Kernel.File.RequestorMode = (Data->RequestorMode != KernelMode);
    Message->Kernel.File.MajorFunction = Data->Iopb->MajorFunction;
    Message->Kernel.File.MinorFunction = Data->Iopb->MinorFunction;
    Message->Kernel.File.Irql = KeGetCurrentIrql();
    Message->Kernel.File.OperationFlags = Data->Iopb->OperationFlags;
    Message->Kernel.File.FltFlags = Data->Flags;
    Message->Kernel.File.IrpFlags = Data->Iopb->IrpFlags;

    Message->Kernel.File.Parameters.Others.Argument1 = Data->Iopb->Parameters.Others.Argument1;
    Message->Kernel.File.Parameters.Others.Argument2 = Data->Iopb->Parameters.Others.Argument2;
    Message->Kernel.File.Parameters.Others.Argument3 = Data->Iopb->Parameters.Others.Argument3;
    Message->Kernel.File.Parameters.Others.Argument4 = Data->Iopb->Parameters.Others.Argument4;
    Message->Kernel.File.Parameters.Others.Argument5 = Data->Iopb->Parameters.Others.Argument5;
    Message->Kernel.File.Parameters.Others.Argument6 = Data->Iopb->Parameters.Others.Argument6;

    if (!Data->Iopb->TargetFileObject)
    {
        return;
    }

    Message->Kernel.File.DeletePending = (Data->Iopb->TargetFileObject->DeletePending != FALSE);
    Message->Kernel.File.Busy = (Data->Iopb->TargetFileObject->Busy != 0);
    Message->Kernel.File.Flags = Data->Iopb->TargetFileObject->Flags;
    Message->Kernel.File.Waiters = Data->Iopb->TargetFileObject->Waiters;
    Message->Kernel.File.CurrentByteOffset = Data->Iopb->TargetFileObject->CurrentByteOffset;
}

/**
 * \brief Fills a message with pre operation information.
 *
 * \param[in,out] Message The message to fill.
 * \param[in] Data The callback data for the operation.
 * \param[in] Options The options for the filter.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpFltFillPreOpMessage(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PKPH_FLT_OPTIONS Options
    )
{
    PMDL mdl;
    PVOID buffer;
    ULONG length;
    KPH_MESSAGE_FIELD_ID fieldId;
    PVOID destBuffer;
    ULONG destLength;
    BOOLEAN truncate;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    KphpFltFillCommonMessage(Message, Data);

    mdl = NULL;
    fieldId = InvalidKphMsgField;
    destBuffer = NULL;
    destLength = 0;
    truncate = TRUE;

    switch (Data->Iopb->MajorFunction)
    {
        case IRP_MJ_CREATE:
        {
            NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

            buffer = Data->Iopb->Parameters.Create.EaBuffer;
            length = Data->Iopb->Parameters.Create.EaLength;
            fieldId = KphMsgFieldEaBuffer;
            break;
        }
        case IRP_MJ_CREATE_NAMED_PIPE:
        {
            buffer = Data->Iopb->Parameters.CreatePipe.Parameters;
            length = sizeof(NAMED_PIPE_CREATE_PARAMETERS);
            destBuffer = &Message->Kernel.File.Pre.CreateNamedPipe.Parameters;
            destLength = sizeof(NAMED_PIPE_CREATE_PARAMETERS);
            break;
        }
        case IRP_MJ_SET_INFORMATION:
        {
            buffer = Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
            length = Data->Iopb->Parameters.SetFileInformation.Length;
            fieldId = KphMsgFieldInformationBuffer;
            break;
        }
        case IRP_MJ_QUERY_EA:
        {
            //
            // Pre operation captures the input query list. Post operation will
            // capture any returned information.
            //
            buffer = Data->Iopb->Parameters.QueryEa.EaList;
            length = Data->Iopb->Parameters.QueryEa.EaListLength;
            fieldId = KphMsgFieldInformationBuffer; // FILE_GET_EA_INFORMATION
            break;
        }
        case IRP_MJ_SET_EA:
        {
            mdl = Data->Iopb->Parameters.SetEa.MdlAddress;
            buffer = Data->Iopb->Parameters.SetEa.EaBuffer;
            length = Data->Iopb->Parameters.SetEa.Length;
            fieldId = KphMsgFieldInformationBuffer; // FILE_FULL_EA_INFORMATION
            break;
        }
        case IRP_MJ_SET_VOLUME_INFORMATION:
        {
            buffer = Data->Iopb->Parameters.SetVolumeInformation.VolumeBuffer;
            length = Data->Iopb->Parameters.SetVolumeInformation.Length;
            fieldId = KphMsgFieldInformationBuffer;
            break;
        }
        case IRP_MJ_DIRECTORY_CONTROL:
        {
            PUNICODE_STRING fileName;

            if (Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY)
            {
                return;
            }

            fileName = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileName;
            if (!fileName)
            {
                return;
            }

            __try
            {
                buffer = fileName->Buffer;
                length = fileName->Length;
                fieldId = KphMsgFieldDestinationFileName;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              INFORMER,
                              "Exception capturing file name: %!STATUS!",
                              GetExceptionCode());
                return;
            }

            truncate = FALSE;
            break;
        }
        case IRP_MJ_FILE_SYSTEM_CONTROL:
        {
            if (Options->EnableFsControlBuffers)
            {
                KphpFltCopyFsControl(Message, Data);
            }
            return;
        }
        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
            if (Options->EnableIoControlBuffers)
            {
                KphpFltCopyIoControl(Message, Data);
            }
            return;
        }
        case IRP_MJ_LOCK_CONTROL:
        {
            buffer = Data->Iopb->Parameters.LockControl.Length;
            length = sizeof(LARGE_INTEGER);
            destBuffer = &Message->Kernel.File.Pre.LockControl.Length;
            destLength = sizeof(LARGE_INTEGER);
            break;
        }
        case IRP_MJ_CREATE_MAILSLOT:
        {
            buffer = Data->Iopb->Parameters.CreateMailslot.Parameters;
            length = sizeof(MAILSLOT_CREATE_PARAMETERS);
            destBuffer = &Message->Kernel.File.Pre.CreateMailslot.Parameters;
            destLength = sizeof(MAILSLOT_CREATE_PARAMETERS);
            break;
        }
        case IRP_MJ_ACQUIRE_FOR_MOD_WRITE:
        {
            buffer = Data->Iopb->Parameters.AcquireForModifiedPageWriter.EndingOffset;
            length = sizeof(LARGE_INTEGER);
            destBuffer = &Message->Kernel.File.Pre.AcquireForModWrite.EndingOffset;
            destLength = sizeof(LARGE_INTEGER);
            break;
        }
        default:
        {
            return;
        }
    }

    KphpFltCopyBuffer(Message,
                      Data,
                      fieldId,
                      FALSE,
                      destBuffer,
                      destLength,
                      mdl,
                      buffer,
                      length,
                      truncate);
}

/**
 * \brief Fills a message with post operation information.
 *
 * \param[in,out] Message The message to fill.
 * \param[in] Data The callback data for the operation.
 * \param[in] Options The options for the filter.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpFltFillPostOpMessage(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PKPH_FLT_OPTIONS Options
    )
{
    PMDL mdl;
    PVOID buffer;
    ULONG length;
    KPH_MESSAGE_FIELD_ID fieldId;

    NPAGED_CODE_DISPATCH_MAX();

    KphpFltFillCommonMessage(Message, Data);

    Message->Kernel.File.Post.IoStatus = Data->IoStatus;

    if (KeGetCurrentIrql() > APC_LEVEL)
    {
        return;
    }

    mdl = NULL;
    fieldId = InvalidKphMsgField;

    switch (Data->Iopb->MajorFunction)
    {
        case IRP_MJ_QUERY_INFORMATION:
        {
            buffer = Data->Iopb->Parameters.QueryFileInformation.InfoBuffer;
            length = (ULONG)Data->IoStatus.Information;
            fieldId = KphMsgFieldInformationBuffer;
            break;
        }
        case IRP_MJ_QUERY_EA:
        {
            //
            // Pre operation captured the input EA list.
            //
            mdl = Data->Iopb->Parameters.QueryEa.MdlAddress;
            buffer = Data->Iopb->Parameters.QueryEa.EaBuffer;
            length = (ULONG)Data->IoStatus.Information;
            fieldId = KphMsgFieldInformationBuffer; // FILE_FULL_EA_INFORMATION
            break;
        }
        case IRP_MJ_QUERY_VOLUME_INFORMATION:
        {
            buffer = Data->Iopb->Parameters.QueryVolumeInformation.VolumeBuffer;
            length = (ULONG)Data->IoStatus.Information;
            fieldId = KphMsgFieldInformationBuffer;
            break;
        }
        case IRP_MJ_DIRECTORY_CONTROL:
        {
            if (!Options->EnableDirControlBuffers)
            {
                return;
            }

            switch (Data->Iopb->MinorFunction)
            {
                case IRP_MN_QUERY_DIRECTORY:
                {
                    //
                    // Pre operation captured the input file name.
                    //
                    mdl = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress;
                    buffer = Data->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;
                    length = (ULONG)Data->IoStatus.Information;
                    fieldId = KphMsgFieldInformationBuffer;
                    break;
                }
                case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
                {
                    buffer = Data->Iopb->Parameters.DirectoryControl.NotifyDirectory.DirectoryBuffer;
                    length = (ULONG)Data->IoStatus.Information;
                    fieldId = KphMsgFieldInformationBuffer;
                    break;
                }
                case IRP_MN_NOTIFY_CHANGE_DIRECTORY_EX:
                {
                    mdl = Data->Iopb->Parameters.DirectoryControl.NotifyDirectoryEx.MdlAddress;
                    buffer = Data->Iopb->Parameters.DirectoryControl.NotifyDirectoryEx.DirectoryBuffer;
                    length = (ULONG)Data->IoStatus.Information;
                    fieldId = KphMsgFieldInformationBuffer;
                    break;
                }
                default:
                {
                    return;
                }
            }
            break;
        }
        case IRP_MJ_FILE_SYSTEM_CONTROL:
        {
            if (Options->EnableFsControlBuffers)
            {
                KphpFltCopyFsControl(Message, Data);
            }
            return;
        }
        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
            if (Options->EnableIoControlBuffers)
            {
                KphpFltCopyIoControl(Message, Data);
            }
            return;
        }
        case IRP_MJ_QUERY_SECURITY:
        {
            mdl = Data->Iopb->Parameters.QuerySecurity.MdlAddress;
            buffer = Data->Iopb->Parameters.QuerySecurity.SecurityBuffer;
            length = (ULONG)Data->IoStatus.Information;
            fieldId = KphMsgFieldInformationBuffer;
            break;
        }
        case IRP_MJ_QUERY_OPEN:
        {
            buffer = Data->Iopb->Parameters.QueryOpen.FileInformation;
            length = (ULONG)Data->IoStatus.Information;
            fieldId = KphMsgFieldInformationBuffer;
            break;
        }
        case IRP_MJ_NETWORK_QUERY_OPEN:
        {
            buffer = Data->Iopb->Parameters.NetworkQueryOpen.NetworkInformation;
            length = sizeof(FILE_NETWORK_OPEN_INFORMATION);
            fieldId = KphMsgFieldInformationBuffer;
            break;
        }
        default:
        {
            return;
        }
    }

    KphpFltCopyBuffer(Message,
                      Data,
                      fieldId,
                      FALSE,
                      NULL,
                      0,
                      mdl,
                      buffer,
                      length,
                      TRUE);
}

/**
 * \brief Handles name tunneling for a given file name information.
 *
 * \details If the file name was tunneled, the re-tunneled file name will be
 * copied into the message, otherwise the original file name will be copied.
 *
 * \param[in,out] Message The message to copy the file name into.
 * \param[in] Data The callback data for the operation.
 * \param[in] FieldId The field to copy the file name into.
 * \param[in] FileNameInfo The file name information to handle.
 *
 * \return TRUE if the file name was tunneled, FALSE otherwise.
 */
_IRQL_requires_max_(APC_LEVEL)
BOOLEAN KphpFltHandleNameTunneling(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PFLT_FILE_NAME_INFORMATION FileNameInfo
    )
{
    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION reTunneledFileNameInfo;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    NT_ASSERT(FileNameInfo->Format == FLT_FILE_NAME_NORMALIZED);

    status = FltGetTunneledName(Data,
                                FileNameInfo,
                                &reTunneledFileNameInfo);
    if (!NT_SUCCESS(status))
    {
        if (NT_SUCCESS(Data->IoStatus.Status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "FltGetTunneledName failed: %!STATUS!",
                          status);
        }

        reTunneledFileNameInfo = FALSE;
    }

    if (!reTunneledFileNameInfo)
    {
        //
        // The file name was not tunneled.
        //
        status = KphMsgDynAddUnicodeString(Message,
                                           FieldId,
                                           &FileNameInfo->Name);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphMsgDynAddUnicodeString failed: %!STATUS!",
                          status);
        }

        return FALSE;
    }

    status = KphMsgDynAddUnicodeString(Message,
                                       FieldId,
                                       &reTunneledFileNameInfo->Name);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

    FltReleaseFileNameInformation(reTunneledFileNameInfo);

    return TRUE;
}

/**
 * \brief Check if name tunneling is relevant for the operation.
 *
 * \param[in] Data The callback data for the operation.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN KphpFltIsNameTunnelingPossible(
    _In_ PFLT_CALLBACK_DATA Data
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    if (Data->Iopb->MajorFunction == IRP_MJ_CREATE)
    {
        return TRUE;
    }
    else if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION)
    {
        switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass)
        {
            case FileRenameInformation:
            case FileRenameInformationBypassAccessCheck:
            case FileRenameInformationEx:
            case FileRenameInformationExBypassAccessCheck:
            case FileLinkInformation:
            case FileLinkInformationBypassAccessCheck:
            case FileLinkInformationEx:
            case FileLinkInformationExBypassAccessCheck:
            {
                return TRUE;
            }
            default:
            {
                break;
            }
        }
    }

    return FALSE;
}

/**
 * \brief Handles name tunneling in the post operation callback.
 *
 * \param[in] Data The callback data for the operation.
 * \param[in,out] Context The completion context for the operation.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpFltPostOpHandleNameTunneling(
    _In_ PFLT_CALLBACK_DATA Data,
    _Inout_ PKPH_FLT_COMPLETION_CONTEXT Context
    )
{
    BOOLEAN tunneledFileName;
    BOOLEAN tunneledDestFileName;

    NPAGED_CODE_DISPATCH_MAX();

    NT_ASSERT(KphpFltIsNameTunnelingPossible(Data));

    tunneledFileName = FALSE;
    tunneledDestFileName = FALSE;

    if (Context->FileNameInfo)
    {
        NT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
        NT_ASSERT(Context->FileNameInfo->Format == FLT_FILE_NAME_NORMALIZED);
        NT_ASSERT((Data->Iopb->MajorFunction == IRP_MJ_CREATE) ||
                  (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION));

        if (KphpFltHandleNameTunneling(Context->Message,
                                       Data,
                                       KphMsgFieldFileName,
                                       Context->FileNameInfo))
        {
            tunneledFileName = TRUE;
        }
    }

    if (Context->DestFileNameInfo)
    {
        NT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
        NT_ASSERT(Context->DestFileNameInfo->Format == FLT_FILE_NAME_NORMALIZED);
        NT_ASSERT(Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION);

        if (KphpFltHandleNameTunneling(Context->Message,
                                       Data,
                                       KphMsgFieldDestinationFileName,
                                       Context->DestFileNameInfo))
        {
            tunneledDestFileName = TRUE;
        }
    }

    if (Data->Iopb->MajorFunction == IRP_MJ_CREATE)
    {
        Context->Message->Kernel.File.Post.Create.TunneledFileName = tunneledFileName;
    }
    else if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION)
    {
        Context->Message->Kernel.File.Post.SetInformation.TunneledFileName = tunneledFileName;
        Context->Message->Kernel.File.Post.SetInformation.TunneledDestinationFileName = tunneledDestFileName;
    }
}

/**
 * \brief Frees a completion context.
 *
 * \param[in] CompletionContext The completion context to free.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpFltFreeCompletionContext(
    _In_ PKPH_FLT_COMPLETION_CONTEXT CompletionContext
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    if (CompletionContext->Message)
    {
        KphFreeNPagedMessage(CompletionContext->Message);
    }

    //
    // N.B. The file name information is released in the post operation should
    // only occur from IRP_MJ_CREATE or IRP_MJ_SET_INFORMATION when the name
    // needs to be possibly re-tunneled. This should only happen at or below
    // APC_LEVEL.
    //

    if (CompletionContext->FileNameInfo)
    {
        NT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
        FltReleaseFileNameInformation(CompletionContext->FileNameInfo);
    }

    if (CompletionContext->DestFileNameInfo)
    {
        NT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
        FltReleaseFileNameInformation(CompletionContext->DestFileNameInfo);
    }

    KphFreeToNPagedLookaside(&KphpFltCompletionContextLookaside,
                             CompletionContext);
}

/**
 * \brief Filter post operation callback for file operations.
 *
 * \param[in] Data The callback data for the operation.
 * \param[in] FltObjects The related objects for the operation.
 * \param[in] CompletionContext The completion context for the operation.
 * \param[in] Flags Filter post operation flags.
 *
 * \return FLT_POSTOP_FINISHED_PROCESSING
 */
_Function_class_(PFLT_POST_OPERATION_CALLBACK)
_IRQL_requires_max_(DISPATCH_LEVEL)
FLT_POSTOP_CALLBACK_STATUS FLTAPI KphpFltPostOp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
    NTSTATUS status;
    ULONG64 sequence;
    KPH_MESSAGE_ID messageId;
    PKPH_FLT_COMPLETION_CONTEXT context;
    PKPH_MESSAGE reply;

    NPAGED_CODE_DISPATCH_MAX();

    sequence = InterlockedIncrementU64(&KphpFltSequence);

    context = CompletionContext;
    reply = NULL;

    if (!context)
    {
        goto Exit;
    }

    if (FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING))
    {
        NT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
        goto Exit;
    }

    //
    // Message was already initialized by the pre operation callback.
    // But we need to update a few things for the post operation callback.
    //
    NT_ASSERT(NT_SUCCESS(KphMsgValidate(context->Message)));
    NT_ASSERT(context->Message->Kernel.File.Sequence == 0);
    NT_ASSERT(context->Message->Header.TimeStamp.QuadPart == 0);

    messageId = KphpFltGetMessageId(Data->Iopb->MajorFunction, TRUE);
    context->Message->Header.MessageId = messageId;
    KeQuerySystemTime(&context->Message->Header.TimeStamp);
    context->Message->Kernel.File.Sequence = sequence;

    if (KphpFltIsNameTunnelingPossible(Data))
    {
        KphpFltPostOpHandleNameTunneling(Data, context);
    }

    KphpFltFillPostOpMessage(context->Message, Data, &context->Options);

    if (context->Options.EnableStackTraces)
    {
        KphCaptureStackInMessage(context->Message);
    }

    if ((Data->Iopb->MajorFunction != IRP_MJ_CREATE) ||
        !context->Options.EnablePostCreateReply)
    {
        KphCommsSendNPagedMessageAsync(context->Message);
        context->Message = NULL;

        goto Exit;
    }

    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    reply = KphAllocateNPagedMessage();
    if (!reply)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphAllocateNPagedMessage failed");

        goto Exit;
    }

    status = KphCommsSendMessage(context->Message, reply);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to send message (%lu): %!STATUS!",
                      (ULONG)context->Message->Header.MessageId,
                      status);

        goto Exit;
    }

    if ((reply->Header.MessageId == KphMsgFilePostCreate) &&
        (reply->Reply.File.Post.Create.Status != STATUS_SUCCESS))
    {
        Data->IoStatus.Status = reply->Reply.File.Post.Create.Status;
        Data->IoStatus.Information = 0;
        FltCancelFileOpen(FltObjects->Instance, FltObjects->FileObject);
    }

Exit:

    if (reply)
    {
        KphFreeNPagedMessage(reply);
    }

    if (context)
    {
        KphpFltFreeCompletionContext(context);
    }

    if (Data->Iopb->MajorFunction == IRP_MJ_CLOSE)
    {
        KphpFltReapFileNameCache(Data, FltObjects);
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}

/**
 * \brief Creates a message from pre the operation callback the post operation.
 *
 * \param[in] Data The callback data for the operation.
 * \param[in] FltObjects The related objects for the operation.
 * \param[in] Options The options for the filter.
 * \param[in] FltFileName The file name information for the operation.
 * \param[in] FltDestFileName The destination file name information, if any.
 * \param[in] Sequence The sequence number for the pre operation.
 * \param[in] TimeStamp The time stamp for the pre operation.
 * \param[out] Context Set to the completion context for the operation.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltPreOpCreateCompletionContext(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PKPH_FLT_OPTIONS Options,
    _In_ PKPH_FLT_FILE_NAME FltFileName,
    _In_ PKPH_FLT_FILE_NAME FltDestFileName,
    _In_ ULONG64 Sequence,
    _In_ PLARGE_INTEGER TimeStamp,
    _Out_ PKPH_FLT_COMPLETION_CONTEXT* CompletionContext
    )
{
    NTSTATUS status;
    PKPH_FLT_COMPLETION_CONTEXT context;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    *CompletionContext = NULL;

    context = KphAllocateFromNPagedLookaside(&KphpFltCompletionContextLookaside);
    if (!context)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphAllocateFromNPagedLookaside failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    context->Options.Flags = Options->Flags;

    //
    // Create the message for the post operation to finish filling in. We
    // will populate what we can in the pre operation callback. In some cases
    // this is necessary since post operation can complete at dispatch.
    //

    context->Message = KphAllocateNPagedMessage();
    if (!context->Message)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphAllocateNPagedMessage failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    KphpFltInitMessage(context->Message,
                       FltObjects,
                       Data->Iopb->MajorFunction,
                       TRUE);

    //
    // Will be populated when the operation completes. Set a sentinel value
    // for the time stamp to indicate that it is not yet valid.
    //
    NT_ASSERT(context->Message->Kernel.File.Sequence == 0);
    context->Message->Header.TimeStamp.QuadPart = 0;

    context->Message->Kernel.File.Post.PreSequence = Sequence;
    context->Message->Kernel.File.Post.PreTimeStamp = *TimeStamp;

    if (Options->EnablePostFileNames)
    {
        //
        // Only if we have to will we fill in the file name information in the post
        // operation (and resolve a possibly tunneled names there). In most cases
        // we will copy the file name information into the post message here.
        //
        if (!KphpFltIsNameTunnelingPossible(Data))
        {
            KphpFltCopyFileName(context->Message, KphMsgFieldFileName, FltFileName);
            KphpFltCopyFileName(context->Message, KphMsgFieldDestinationFileName, FltDestFileName);
        }
        else if (Data->Iopb->MajorFunction == IRP_MJ_CREATE)
        {
            if ((FltFileName->Type == KphFltFileNameTypeNameInfo) &&
                (FltFileName->NameInfo->Format == FLT_FILE_NAME_NORMALIZED))
            {
                context->FileNameInfo = FltFileName->NameInfo;
                FltReferenceFileNameInformation(context->FileNameInfo);
            }
            else
            {
                KphpFltCopyFileName(context->Message, KphMsgFieldFileName, FltFileName);
            }
        }
        else
        {
            NT_ASSERT(Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION);

            if ((FltFileName->Type == KphFltFileNameTypeNameInfo) &&
                (FltFileName->NameInfo->Format == FLT_FILE_NAME_NORMALIZED))
            {
                context->FileNameInfo = FltFileName->NameInfo;
                FltReferenceFileNameInformation(context->FileNameInfo);
            }
            else
            {
                KphpFltCopyFileName(context->Message, KphMsgFieldFileName, FltFileName);
            }

            if ((FltDestFileName->Type == KphFltFileNameTypeNameInfo) &&
                (FltDestFileName->NameInfo->Format == FLT_FILE_NAME_NORMALIZED))
            {
                context->DestFileNameInfo = FltDestFileName->NameInfo;
                FltReferenceFileNameInformation(context->DestFileNameInfo);
            }
            else
            {
                KphpFltCopyFileName(context->Message, KphMsgFieldDestinationFileName, FltDestFileName);
            }
        }
    }

    *CompletionContext = context;
    context = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (context)
    {
        KphpFltFreeCompletionContext(context);
    }

    return status;
}

/**
 * \brief Sends a pre operation message.
 *
 * \param[in,out] Data The callback data for the operation.
 * \param[in] FltObjects The related objects for the operation.
 * \param[in] Options The options for the filter.
 * \param[in] FltFileName The file name information for the operation.
 * \param[in] FltDestFileName The destination file name information, if any.
 * \param[in] Sequence The sequence number for the operation.
 * \param[out] TimeStamp Set to the time stamp for the pre operation.
 *
 * \return FLT_PREOP_SUCCESS_NO_CALLBACK or FLT_PREOP_COMPLETE
 */
_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS KphpFltPreOpSend(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PKPH_FLT_OPTIONS Options,
    _In_ PKPH_FLT_FILE_NAME FltFileName,
    _In_ PKPH_FLT_FILE_NAME FltDestFileName,
    _In_ ULONG64 Sequence,
    _Out_ PLARGE_INTEGER TimeStamp
    )
{
    FLT_PREOP_CALLBACK_STATUS callbackStatus;
    NTSTATUS status;
    PKPH_MESSAGE message;
    PKPH_MESSAGE reply;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
    reply = NULL;

    message = KphAllocateNPagedMessage();
    if (!message)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphAllocateNPagedMessage failed");

        KeQuerySystemTime(TimeStamp);
        goto Exit;
    }

    KphpFltInitMessage(message, FltObjects, Data->Iopb->MajorFunction, FALSE);

    *TimeStamp = message->Header.TimeStamp;

    message->Kernel.File.Sequence = Sequence;

    KphpFltCopyFileName(message, KphMsgFieldFileName, FltFileName);
    KphpFltCopyFileName(message, KphMsgFieldDestinationFileName, FltDestFileName);

    //
    // The post operation callback will resolve this name and note if it
    // was a re-tunneled file name. For the pre operation callback we can
    // only denote in the message that the file name is possibly tunneled.
    //
    if (!KphpFltIsNameTunnelingPossible(Data))
    {
        NOTHING;
    }
    else if (Data->Iopb->MajorFunction == IRP_MJ_CREATE)
    {
        if ((FltFileName->Type == KphFltFileNameTypeNameInfo) &&
            (FltFileName->NameInfo->Format == FLT_FILE_NAME_NORMALIZED))
        {
            message->Kernel.File.Pre.Create.MaybeTunneledFileName = TRUE;
        }
    }
    else
    {
        NT_ASSERT(Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION);

        if ((FltFileName->Type == KphFltFileNameTypeNameInfo) &&
            (FltFileName->NameInfo->Format == FLT_FILE_NAME_NORMALIZED))
        {
            message->Kernel.File.Pre.SetInformation.MaybeTunneledFileName = TRUE;
        }

        if ((FltDestFileName->Type == KphFltFileNameTypeNameInfo) &&
            (FltDestFileName->NameInfo->Format == FLT_FILE_NAME_NORMALIZED))
        {
            message->Kernel.File.Pre.SetInformation.MaybeTunneledDestinationFileName = TRUE;
        }
    }

    KphpFltFillPreOpMessage(message, Data, Options);

    if (Options->EnableStackTraces)
    {
        KphCaptureStackInMessage(message);
    }

    if ((Data->Iopb->MajorFunction != IRP_MJ_CREATE) ||
        !Options->EnablePreCreateReply)
    {
        KphCommsSendNPagedMessageAsync(message);
        message = NULL;

        goto Exit;
    }

    reply = KphAllocateNPagedMessage();
    if (!reply)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphAllocateNPagedMessage failed");

        goto Exit;
    }

    status = KphCommsSendMessage(message, reply);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to send message (%lu): %!STATUS!",
                      (ULONG)message->Header.MessageId,
                      status);

        goto Exit;
    }

    if ((reply->Header.MessageId == KphMsgFilePreCreate) &&
        (reply->Reply.File.Pre.Create.Status != STATUS_SUCCESS))
    {
        Data->IoStatus.Status = reply->Reply.File.Pre.Create.Status;
        Data->IoStatus.Information = 0;
        callbackStatus = FLT_PREOP_COMPLETE;
    }

Exit:

    if (reply)
    {
        KphFreeNPagedMessage(reply);
    }

    if (message)
    {
        KphFreeNPagedMessage(message);
    }

    return callbackStatus;
}

/**
 * \brief Handles a request to perform an action inside of the filter.
 *
 * \param[in,out] Data The callback data for the operation.
 * \param[in] FltObjects The related objects for the operation.
 */
VOID KphpFltRequestHandler(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects
    )
{
    PKPH_THREAD_CONTEXT thread;

    //
    // KphQueryVirtualMemory will use this to create a data section object.
    // It will do this by issuing a MemoryMappedFilenameInformation. This
    // results in an IRP_MJ_QUERY_INFORMATION with FileNameInformation. And will
    // have previously set the one of the "TLS slots" to the address of the
    // KphQueryVirtualMemory stack to pass information to and from this call.
    //

    if ((Data->Iopb->MajorFunction != IRP_MJ_QUERY_INFORMATION) ||
        (Data->Iopb->Parameters.QueryFileInformation.FileInformationClass != FileNameInformation))
    {
        return;
    }

    thread = KphGetCurrentThreadContext();
    if (!thread)
    {
        return;
    }

    if (thread->VmTlsCreateDataSection)
    {
        PKPH_VM_TLS_CREATE_DATA_SECTION tls;
        NTSTATUS status;
        OBJECT_ATTRIBUTES objectAttributes;
        PVOID sectionObject;

        tls = thread->VmTlsCreateDataSection;

        thread->VmTlsCreateDataSection = NULL;

        InitializeObjectAttributes(&objectAttributes,
                                   NULL,
                                   (tls->AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                   NULL,
                                   NULL);

        status = FsRtlCreateSectionForDataScan(&tls->SectionHandle,
                                               &sectionObject,
                                               &tls->SectionFileSize,
                                               FltObjects->FileObject,
                                               SECTION_QUERY | SECTION_MAP_READ,
                                               &objectAttributes,
                                               NULL,
                                               PAGE_READONLY,
                                               SEC_COMMIT,
                                               0);
        if (NT_SUCCESS(status))
        {
            ObDereferenceObject(sectionObject);
        }

        tls->Status = status;
    }
    else if (thread->VmTlsMappedInformation)
    {
        PKPH_MEMORY_MAPPED_INFORMATION tls;

        tls = thread->VmTlsMappedInformation;

        thread->VmTlsMappedInformation = NULL;

        tls->FileObject = FltObjects->FileObject;
        tls->SectionObjectPointers = FltObjects->FileObject->SectionObjectPointer;
        if (FltObjects->FileObject->SectionObjectPointer)
        {
            tls->DataControlArea = FltObjects->FileObject->SectionObjectPointer->DataSectionObject;
            tls->SharedCacheMap = FltObjects->FileObject->SectionObjectPointer->SharedCacheMap;
            tls->ImageControlArea = FltObjects->FileObject->SectionObjectPointer->ImageSectionObject;
            tls->UserWritableReferences = MmDoesFileHaveUserWritableReferences(FltObjects->FileObject->SectionObjectPointer);
        }
    }

    KphDereferenceObject(thread);
}

/**
 * \brief Filter pre operation callback for file operations.
 *
 * \param[in,out] Data The callback data for the operation.
 * \param[in] FltObjects The related objects for the operation.
 * \param[out] CompletionContext Set to the completion context if necessary.
 *
 * \return An appropriate callback status depending on the a post operation
 * is needed or the pre operation should be completed.
 */
_Function_class_(PFLT_PRE_OPERATION_CALLBACK)
_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS FLTAPI KphpFltPreOp(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID* CompletionContext
    )
{
    ULONG64 sequence;
    FLT_PREOP_CALLBACK_STATUS callbackStatus;
    KPH_FLT_OPTIONS options;
    NTSTATUS status;
    KPH_FLT_FILE_NAME fltFileName;
    KPH_FLT_FILE_NAME fltDestFileName;
    LARGE_INTEGER timeStamp;

    NPAGED_CODE_APC_MAX_FOR_PAGING_IO();

    *CompletionContext = NULL;

    KphpFltRequestHandler(Data, FltObjects);

    sequence = InterlockedIncrementU64(&KphpFltSequence);
    callbackStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    KphpFltZeroFileName(&fltFileName);
    KphpFltZeroFileName(&fltDestFileName);

    options = KphpFltGetOptions(Data);

    if (!options.PreEnabled && !options.PostEnabled)
    {
        goto Exit;
    }

    if (!options.EnablePagingIo &&
        FlagOn(Data->Iopb->IrpFlags, IRP_PAGING_IO))
    {
        goto Exit;
    }

    if (!options.EnableSyncPagingIo &&
        FlagOn(Data->Iopb->IrpFlags, IRP_SYNCHRONOUS_PAGING_IO))
    {
        goto Exit;
    }

    //
    // Either pre or post is enabled, gather what is needed.
    //

    status = KphpFltGetFileName(Data, FltObjects, &fltFileName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphpFltGetFileName failed: %!STATUS!",
                      status);
    }

    if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION)
    {
        switch (Data->Iopb->Parameters.SetFileInformation.FileInformationClass)
        {
            case FileRenameInformation:
            case FileRenameInformationBypassAccessCheck:
            case FileRenameInformationEx:
            case FileRenameInformationExBypassAccessCheck:
            case FileLinkInformation:
            case FileLinkInformationBypassAccessCheck:
            case FileLinkInformationEx:
            case FileLinkInformationExBypassAccessCheck:
            {
                status = KphpFltGetDestFileName(Data,
                                                FltObjects,
                                                &fltDestFileName);
                if (!NT_SUCCESS(status))
                {
                    KphTracePrint(TRACE_LEVEL_VERBOSE,
                                  INFORMER,
                                  "KphpFltGetDestFileName failed: %!STATUS!",
                                  status);
                }

                break;
            }
        }
    }

    if (options.PreEnabled)
    {
        callbackStatus = KphpFltPreOpSend(Data,
                                          FltObjects,
                                          &options,
                                          &fltFileName,
                                          &fltDestFileName,
                                          sequence,
                                          &timeStamp);

        NT_ASSERT((callbackStatus == FLT_PREOP_SUCCESS_NO_CALLBACK) ||
                  (callbackStatus == FLT_PREOP_COMPLETE));

        if (callbackStatus == FLT_PREOP_COMPLETE)
        {
            goto Exit;
        }
    }
    else
    {
        //
        // Pre operations aren't enabled, but we need to grab the time stamp
        // for the post operation to use.
        //
        KeQuerySystemTime(&timeStamp);
    }

    if (options.PostEnabled)
    {
        PKPH_FLT_COMPLETION_CONTEXT context;

        status = KphpFltPreOpCreateCompletionContext(Data,
                                                     FltObjects,
                                                     &options,
                                                     &fltFileName,
                                                     &fltDestFileName,
                                                     sequence,
                                                     &timeStamp,
                                                     &context);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphpFltPreOpCreatePostOpMessage failed: %!STATUS!",
                          status);

            goto Exit;
        }

        if (Data->Iopb->MajorFunction == IRP_MJ_SHUTDOWN)
        {
            //
            // Post operation for IRP_MJ_SHUTDOWN is not supported, we do not
            // register for one, fake it here by directly calling the post
            // operation handler.
            //
            KphpFltPostOp(Data, FltObjects, context, 0);
        }
        else
        {
            *CompletionContext = context;
            callbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
        }
    }

Exit:

    KphpFltReleaseFileName(&fltDestFileName);
    KphpFltReleaseFileName(&fltFileName);

    NT_ASSERT((callbackStatus == FLT_PREOP_SUCCESS_NO_CALLBACK) ||
              (callbackStatus == FLT_PREOP_SUCCESS_WITH_CALLBACK) ||
              (callbackStatus == FLT_PREOP_COMPLETE));

    if ((callbackStatus != FLT_PREOP_SUCCESS_WITH_CALLBACK) &&
        (Data->Iopb->MajorFunction == IRP_MJ_CLOSE))
    {
        NT_ASSERT(callbackStatus != FLT_PREOP_COMPLETE);
        //
        // N.B. We always require a post operation callback for IRP_MJ_CLOSE
        // in order to do reaping (see: KphpFltReapFileNameCache).
        //
        callbackStatus = FLT_PREOP_SUCCESS_WITH_CALLBACK;
    }

    return callbackStatus;
}

PAGED_FILE();

/**
 * \brief Cleans up the file operation filter.
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFltCleanupFileOp(
    VOID
    )
{
    PAGED_CODE_PASSIVE();

    if (!KphpFltOpInitialized)
    {
        return;
    }

    KphDeleteNPagedLookaside(&KphpFltCompletionContextLookaside);
}

/**
 * \brief Initializes the file operation filter.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFltInitializeFileOp(
    VOID
    )
{
    PAGED_CODE_PASSIVE();

    KphInitializeNPagedLookaside(&KphpFltCompletionContextLookaside,
                                 sizeof(KPH_FLT_COMPLETION_CONTEXT),
                                 KPH_TAG_FLT_COMPLETION_CONTEXT);

    KphpFltOpInitialized = TRUE;
}

//
// These are some ugly compile time asserts intentionally hidden at the bottom
// of the file. Essentially, this is asserting that FLT_PARAMETERS has not
// changed enough to update KPHM_FILE_PARAMETERS. Realistically it should never
// change, but they're here for posterity.
//

C_ASSERT(sizeof(KPHM_FILE_PARAMETERS) == sizeof(FLT_PARAMETERS));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, Create) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Create));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, CreateNamedPipe) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       CreatePipe));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, CreateMailslot) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       CreateMailslot));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, Read) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Read));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, Write) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Write));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QueryInformation) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       QueryFileInformation));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SetInformation) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       SetFileInformation));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QueryEa) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       QueryEa));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SetEa) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       SetEa));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QueryVolumeInformation) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       QueryVolumeInformation));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SetVolumeInformation) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       SetVolumeInformation));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, DirectoryControl) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       DirectoryControl));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, FileSystemControl) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       FileSystemControl));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, DeviceControl) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       DeviceIoControl));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, LockControl) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       LockControl));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QuerySecurity) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       QuerySecurity));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SetSecurity) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       SetSecurity));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SystemControl) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       WMI));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QueryQuota) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       QueryQuota));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SetQuota) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       SetQuota));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, Pnp) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Pnp));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, AcquireForSectionSync) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       AcquireForSectionSynchronization));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, AcquireForModWrite) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       AcquireForModifiedPageWriter));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, ReleaseForModWrite) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       ReleaseForModifiedPageWriter));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QueryOpen) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       QueryOpen));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, FastIoCheckIfPossible) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       FastIoCheckIfPossible));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, NetworkQueryOpen) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       NetworkQueryOpen));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, MdlRead) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       MdlRead));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, MdlReadComplete) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       MdlReadComplete));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, PrepareMdlWrite) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       PrepareMdlWrite));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, MdlWriteComplete) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       MdlWriteComplete));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, VolumeMount) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       MountVolume));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, Others) ==
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));

//
// N.B. We make an optimization that we only need to copy these 6 arguments
// when copying the parameter data into the message. Any of these asserts fire
// we must update KPHM_FILE_PARAMETERS.
//

C_ASSERT(RTL_SIZEOF_THROUGH_FIELD(FLT_PARAMETERS, Others.Argument1) == 0x08);
C_ASSERT(RTL_SIZEOF_THROUGH_FIELD(FLT_PARAMETERS, Others.Argument2) == 0x10);
C_ASSERT(RTL_SIZEOF_THROUGH_FIELD(FLT_PARAMETERS, Others.Argument3) == 0x18);
C_ASSERT(RTL_SIZEOF_THROUGH_FIELD(FLT_PARAMETERS, Others.Argument4) == 0x20);
C_ASSERT(RTL_SIZEOF_THROUGH_FIELD(FLT_PARAMETERS, Others.Argument5) == 0x28);
C_ASSERT(RTL_SIZEOF_THROUGH_FIELD(FLT_PARAMETERS, Others.Argument6) == 0x30);
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, Create) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, CreateNamedPipe) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, CreateMailslot) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, Read) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, Write) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QueryInformation) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SetInformation) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QueryEa) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SetEa) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QueryVolumeInformation) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SetVolumeInformation) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, DirectoryControl) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, FileSystemControl) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, DeviceControl) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, LockControl) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QuerySecurity) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SetSecurity) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SystemControl) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QueryQuota) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, SetQuota) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, Pnp) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, AcquireForSectionSync) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, AcquireForModWrite) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, ReleaseForModWrite) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, QueryOpen) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, FastIoCheckIfPossible) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, NetworkQueryOpen) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, MdlRead) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, MdlReadComplete) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, PrepareMdlWrite) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, MdlWriteComplete) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));
C_ASSERT(RTL_FIELD_SIZE(KPHM_FILE_PARAMETERS, VolumeMount) <=
         RTL_FIELD_SIZE(FLT_PARAMETERS,       Others));

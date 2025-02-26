/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpRingBufferType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();
KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpRingBufferTypeName = RTL_CONSTANT_STRING(L"KphRingBuffer");
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Reserves a ring buffer entry.
 *
 * \brief This routine reserves space in the ring buffer to be written to. On a
 * successful call to this routine the returned buffer is marked busy and the
 * caller must commit the buffer with KphCommitRingBuffer or discard it with
 * KphDiscardRingBuffer.
 *
 * \param[in] Ring Pointer to a buffer object.
 * \param[in] Length The length of the buffer to reserve.
 *
 * \return Reserved buffer, null on failure.
 */
_Return_allocatesMem_size_(Length)
PVOID KphReserveRingBuffer(
    _In_ PKPH_RING_BUFFER Ring,
    _In_ ULONG Length
    )
{
    PVOID buffer;
    KIRQL previousIrql;
    KLOCK_QUEUE_HANDLE lockHandle;
    ULONG consumerPos;
    ULONG producerPos;
    ULONG remainingLength;
    ULONG requiredLength;
    ULONG alignment;
    PKPH_RING_HEADER headerPointer;
    KPH_RING_HEADER header;

    //
    // Maximum reserve accounts for the maximum encoded length bits, additional
    // headers, and any necessary alignment.
    //
    if (Length > KPH_RING_BUFFER_RESERVE_MAX)
    {
        return NULL;
    }

    buffer = NULL;

    previousIrql = KeGetCurrentIrql();
    if (previousIrql >= DISPATCH_LEVEL)
    {
        KeAcquireInStackQueuedSpinLockAtDpcLevel(&Ring->ProducerLock,
                                                 &lockHandle);
    }
    else
    {
        KeAcquireInStackQueuedSpinLock(&Ring->ProducerLock, &lockHandle);
    }

    consumerPos = ReadULongAcquire(Ring->ConsumerPos);
    producerPos = ReadULongNoFence(Ring->ProducerPos);

    if (producerPos >= consumerPos)
    {
        remainingLength = (Ring->Length - producerPos);
    }
    else
    {
        //
        // Producer position wrapped.
        //
        remainingLength = (consumerPos - producerPos);
    }

    if (!NT_VERIFY(remainingLength <= Ring->Length))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Ring buffer position overflow: %lu %lu %lu",
                      producerPos,
                      consumerPos,
                      Ring->Length);

        goto Exit;
    }

    requiredLength = (Length + KPH_RING_BUFFER_HEADER_SIZE);
    requiredLength = ALIGN_UP_BY(requiredLength, MEMORY_ALLOCATION_ALIGNMENT);
    alignment = (requiredLength - Length - KPH_RING_BUFFER_HEADER_SIZE);

    //
    // Always reserve an extra header for the reset marker.
    //
    requiredLength += KPH_RING_BUFFER_HEADER_SIZE;

    if (requiredLength > remainingLength)
    {
        if ((producerPos < consumerPos) || (requiredLength > consumerPos))
        {
            //
            // Ring buffer exhausted.
            //
            goto Exit;
        }

        //
        // There is sufficient space to wrap, do so now.
        //

        if (!NT_VERIFY(remainingLength >= KPH_RING_BUFFER_HEADER_SIZE))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Ring buffer accounting failure: %lu %lu %lu",
                          producerPos,
                          consumerPos,
                          Ring->Length);

            goto Exit;
        }

        headerPointer = Add2Ptr(Ring->Buffer, producerPos);
        header.Value = 0;
        header.Reset = TRUE;

        WriteULong64Release(&headerPointer->Value, header.Value);
        producerPos = 0;
    }

    headerPointer = Add2Ptr(Ring->Buffer, producerPos);
    buffer = Add2Ptr(headerPointer, KPH_RING_BUFFER_HEADER_SIZE);

    header.Value = 0;
    header.Length = Length;
    header.Busy = TRUE;
    header.Alignment = alignment;

    producerPos += (requiredLength - KPH_RING_BUFFER_HEADER_SIZE);

    //
    // Sync with the consumer.
    //
    WriteULong64Release(&headerPointer->Value, header.Value);
    WriteULongRelease(Ring->ProducerPos, producerPos);

Exit:

    if (previousIrql >= DISPATCH_LEVEL)
    {
        KeReleaseInStackQueuedSpinLockFromDpcLevel(&lockHandle);
    }
    else
    {
        KeReleaseInStackQueuedSpinLock(&lockHandle);
    }

    return buffer;
}

/**
 * \brief Helper routine to commits a previously reserved ring buffer entry.
 *
 * \param[in] Ring Pointer to a ring buffer object.
 * \param[in] Buffer Pointer to the buffer to commit.
 * \param[in] Discard If TRUE the buffer is discarded, if FALSE is it committed.
 */
VOID KphpCommitRingBuffer(
    _In_ PKPH_RING_BUFFER Ring,
    _In_aliasesMem_ PVOID Buffer,
    _In_ BOOLEAN Discard
    )
{
    PKPH_RING_HEADER headerPointer;
    KPH_RING_HEADER header;

    headerPointer = Add2Ptr(Buffer, -KPH_RING_BUFFER_HEADER_SIZE);

    header.Value = ReadULong64NoFence(&headerPointer->Value);
    header.Busy = FALSE;
    header.Discard = !!Discard;

    WriteULong64Release(&headerPointer->Value, header.Value);

    if (Ring->Event)
    {
        ULONG consumerPos;

        consumerPos = ReadULongAcquire(Ring->ConsumerPos);

        if (Add2Ptr(Ring->Buffer, consumerPos) == headerPointer)
        {
            KeSetEvent(Ring->Event, IO_NO_INCREMENT, FALSE);
        }
    }
}

/**
 * \brief Commits a previously reserved ring buffer entry.
 *
 * \details After this call the busy ring buffer entry is made available for
 * for the consumer to process.
 *
 * \param[in] Ring Pointer to a ring buffer object.
 * \param[in] Buffer Pointer to the buffer to commit, previously returned from
 * KphReserveRingBuffer.
 */
VOID KphCommitRingBuffer(
    _In_ PKPH_RING_BUFFER Ring,
    _In_freesMem_ PVOID Buffer
    )
{
    KphpCommitRingBuffer(Ring, Buffer, FALSE);
}

/**
 * \brief Discards a previously reserved ring buffer entry.
 *
 * \details After this call the busy ring buffer entry is marked as discarded
 * the consumer should skip over it when processing the ring buffer.
 *
 * \param[in] Ring Pointer to a ring buffer object.
 * \param[in] Buffer Pointer to the buffer to discard, previously returned from
 * KphReserveRingBuffer.
 */
VOID KphDiscardRingBuffer(
    _In_ PKPH_RING_BUFFER Ring,
    _In_freesMem_ PVOID Buffer
    )
{
    KphpCommitRingBuffer(Ring, Buffer, TRUE);
}

KPH_PAGED_FILE();

/**
 * \brief Creates a ring buffer section.
 *
 * \details The section is created in a way that restricts read or write access
 * to the ring buffer section by user mode. Kernel mode is always permitted to
 * read or write to the section. The kernel mapping is locked in the system
 * virtual address space. This routine does not map the section into user mode.
 * It does provide the section object to be mapped to user mode by the caller.
 * The method for creating the sections ensures appropriate memory protection is
 * applied to the related section objects and that an eventual mapping into
 * user mode can be paged out if necessary.
 *
 * \param[in] Section Pointer to a ring buffer section to populate.
 * \param[in] Length The length of the ring buffer section.
 * \param[in] PageProtection The page protection for the ring buffer section.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpCreateRingBufferSection(
    _Out_ PKPH_RING_SECTION Section,
    _In_ ULONG Length,
    _In_ ULONG PageProtection
    )
{
    NTSTATUS status;
    ULONG sectionAccess;
    LARGE_INTEGER maximumSize;
    SIZE_T viewSize;
    PVOID kernelMappedBase;

    KPH_PAGED_CODE_PASSIVE();

    kernelMappedBase = NULL;

    if (PageProtection == PAGE_READONLY)
    {
        sectionAccess = SECTION_MAP_READ;
    }
    else if (PageProtection == PAGE_READWRITE)
    {
        sectionAccess = SECTION_MAP_READ | SECTION_MAP_WRITE;
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    maximumSize.QuadPart = Length;

    status = MmCreateSection(&Section->SectionObject,
                             sectionAccess,
                             NULL,
                             &maximumSize,
                             PageProtection,
                             SEC_COMMIT,
                             NULL,
                             NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "MmCreateSection failed: %!STATUS!",
                      status);

        Section->SectionObject = NULL;
        goto Exit;
    }

    kernelMappedBase = NULL;
    viewSize = 0;

    status = MmMapViewInSystemSpace(Section->SectionObject,
                                    &kernelMappedBase,
                                    &viewSize);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "MmMapViewInSystemSpace failed: %!STATUS!",
                      status);

        kernelMappedBase = NULL;
        goto Exit;
    }

    NT_ASSERT((viewSize >= Length) && (viewSize <= ULONG_MAX));

    Section->Mdl = IoAllocateMdl(kernelMappedBase,
                                 (ULONG)viewSize,
                                 FALSE,
                                 FALSE,
                                 NULL);
    if (!Section->Mdl)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, GENERAL, "IoAllocateMdl failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    __try
    {
        MmProbeAndLockPages(Section->Mdl, KernelMode, IoReadAccess);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        IoFreeMdl(Section->Mdl);
        Section->Mdl = NULL;

        status = GetExceptionCode();

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "MmProbeAndLockPages failed: %!STATUS!",
                      status);

        goto Exit;
    }

    Section->KernelBase = KphGetSystemAddressForMdl(Section->Mdl,
                                                    NormalPagePriority);
    if (!Section->KernelBase)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "MmGetSystemAddressForMdlSafe failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    RtlZeroMemory(Section->KernelBase, viewSize);

    status = STATUS_SUCCESS;

Exit:

    if (kernelMappedBase)
    {
        MmUnmapViewInSystemSpace(kernelMappedBase);
    }

    return status;
}

/**
 * \brief Deletes a ring buffer section.
 *
 * \param[in] Section Pointer to a ring buffer section to delete.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpDeleteRingBufferSection(
    _In_ PKPH_RING_SECTION Section
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Section->Mdl)
    {
        MmUnlockPages(Section->Mdl);
        IoFreeMdl(Section->Mdl);
    }

    if (Section->SectionObject)
    {
        ObDereferenceObject(Section->SectionObject);
    }
}

/**
 * \brief Allocates a ring buffer object.
 *
 * \param[in] Size The size of the ring buffer object to allocate.
 *
 * \return Allocated ring buffer object, null on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateRingBuffer(
    _In_ SIZE_T Size
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphAllocateNPaged(Size, KPH_TAG_RING_BUFFER);
}

/**
 * \brief Deletes a ring buffer object.
 *
 * \param[in,out] Object Pointer to a ring buffer object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpDeleteRingBuffer(
    _Inout_ PVOID Object
    )
{
    PKPH_RING_BUFFER ring;

    KPH_PAGED_CODE_PASSIVE();

    ring = Object;

    KphpDeleteRingBufferSection(&ring->ProducerSection);
    KphpDeleteRingBufferSection(&ring->ConsumerSection);

    if (ring->Event)
    {
        ObDereferenceObject(ring->Event);
    }

    ObDereferenceObject(ring->Process);
}

/**
 * \brief Frees a ring buffer object.
 *
 * \param[in] Object Pointer to a ring buffer object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpFreeRingBuffer(
    _In_freesMem_ PVOID Object
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphFree(Object, KPH_TAG_RING_BUFFER);
}

/**
 * \brief Creates a ring buffer object.
 *
 * \details This ring buffer is a multiple-producer single-consumer ring buffer.
 * The producer (kernel mode) populates to the ring buffer and single consumer
 * (in user mode) processes the data written to the buffer. The producer may
 * reserve, write, and commit to the ring buffer from an arbitrary thread.
 *
 * \param[out] Ring Receives the ring buffer object.
 * \param[out] User Receives the user mode ring buffer information.
 * \param[in] Length Desired length of the ring buffer.
 * \param[in] Event Optional event object to an event to signal when new data is
 * committed and the consumer was previously caught up with processing. Must be
 * an ExEventObjectType as a reference is taken to the object.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCreateRingBuffer(
    _Out_ PKPH_RING_BUFFER* Ring,
    _Out_ PKPH_RING_BUFFER_USER User,
    _In_ ULONG Length,
    _In_opt_ PKEVENT Event,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_RING_BUFFER ring;
    PVOID userProducerBase;
    PVOID userConsumerBase;
    ULONG sectionLength;
    ULONG roundedLength;
    ULONG bufferLength;
    PKPH_RING_PRODUCER_BLOCK producer;
    PKPH_RING_CONSUMER_BLOCK consumer;
    LARGE_INTEGER sectionOffset;
    SIZE_T viewSize;

    KPH_PAGED_CODE_PASSIVE();

    *Ring = NULL;

    ring = NULL;
    userProducerBase = NULL;
    userConsumerBase = NULL;

    if (Event && (ObGetObjectType(Event) != *ExEventObjectType))
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(User, KPH_RING_BUFFER_USER);
            RtlZeroVolatileMemory(User, sizeof(KPH_RING_BUFFER_USER));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        RtlZeroMemory(User, sizeof(KPH_RING_BUFFER_USER));
    }

    status = RtlULongAdd(Length,
                         FIELD_OFFSET(KPH_RING_PRODUCER_BLOCK, Buffer),
                         &sectionLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    roundedLength = ROUND_TO_PAGES(sectionLength);
    if (roundedLength < sectionLength)
    {
        status = STATUS_INTEGER_OVERFLOW;
        goto Exit;
    }

    sectionLength = roundedLength;
    bufferLength = sectionLength;
    bufferLength -= FIELD_OFFSET(KPH_RING_PRODUCER_BLOCK, Buffer);

    status = KphCreateObject(KphpRingBufferType,
                             sizeof(KPH_RING_BUFFER),
                             &ring,
                             NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        goto Exit;
    }

    ring->Process = PsGetCurrentProcess();
    ObReferenceObject(ring->Process);

    KeInitializeSpinLock(&ring->ProducerLock);

    if (Event)
    {
        ring->Event = Event;
        ObReferenceObject(ring->Event);
    }

    status = KphpCreateRingBufferSection(&ring->ProducerSection,
                                         sectionLength,
                                         PAGE_READONLY);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphpCreateRingBufferSection failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpCreateRingBufferSection(&ring->ConsumerSection,
                                         sizeof(KPH_RING_CONSUMER_BLOCK),
                                         PAGE_READWRITE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphpCreateRingBufferSection failed: %!STATUS!",
                      status);

        goto Exit;
    }

    producer = ring->ProducerSection.KernelBase;
    consumer = ring->ConsumerSection.KernelBase;

    producer->Length = bufferLength;

    ring->ProducerPos = &producer->Position;
    ring->ConsumerPos = &consumer->Position;
    ring->Length = bufferLength;
    ring->Buffer = producer->Buffer;

    //
    // Set up the user mode portion of the ring buffer.
    //

    sectionOffset.QuadPart = 0;
    viewSize = 0;

    status = MmMapViewOfSection(ring->ProducerSection.SectionObject,
                                PsGetCurrentProcess(),
                                &userProducerBase,
                                0,
                                0,
                                &sectionOffset,
                                &viewSize,
                                ViewUnmap,
                                0,
                                PAGE_READONLY);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "MmMapViewOfSection failed: %!STATUS!",
                      status);

        userProducerBase = NULL;
        goto Exit;
    }

    sectionOffset.QuadPart = 0;
    viewSize = 0;

    status = MmMapViewOfSection(ring->ConsumerSection.SectionObject,
                                PsGetCurrentProcess(),
                                &userConsumerBase,
                                0,
                                0,
                                &sectionOffset,
                                &viewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "MmMapViewOfSection failed: %!STATUS!",
                      status);

        userConsumerBase = NULL;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            User->Consumer = userConsumerBase;
            User->Producer = userProducerBase;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        User->Consumer = userProducerBase;
        User->Producer = userConsumerBase;
    }

    userProducerBase = NULL;
    userConsumerBase = NULL;

    *Ring = ring;
    ring = NULL;

    status = STATUS_SUCCESS;

Exit:

    if (userConsumerBase)
    {
        MmUnmapViewOfSection(PsGetCurrentProcess(), userConsumerBase);
    }

    if (userProducerBase)
    {
        MmUnmapViewOfSection(PsGetCurrentProcess(), userProducerBase);
    }

    if (ring)
    {
        KphDereferenceObject(ring);
    }

    return status;
}

/**
 * \brief Initializes the ring buffer infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeRingBuffer(
    VOID
    )
{
    KPH_OBJECT_TYPE_INFO typeInfo;

    KPH_PAGED_CODE_PASSIVE();

    typeInfo.Allocate = KphpAllocateRingBuffer;
    typeInfo.Initialize = NULL;
    typeInfo.Delete = KphpDeleteRingBuffer;
    typeInfo.Free = KphpFreeRingBuffer;
    typeInfo.Flags = 0;

    KphCreateObjectType(&KphpRingBufferTypeName,
                        &typeInfo,
                        &KphpRingBufferType);
}

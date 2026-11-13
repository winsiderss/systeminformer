/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2022-2026
 *
 */

#include <kph.h>
#include <kphdyndata.h>
#include <kphhw.h>

#include <trace.h>

typedef struct _KPH_DYN_INIT
{
    PKPH_DYN_CONFIG Config;
    PKPH_DYN_KERNEL_FIELDS Kernel;
    PKPH_DYN_LXCORE_FIELDS Lxcore;
    PKPH_DYN_HW_SECTION Hardware;
    ULONG HardwareLength;
} KPH_DYN_INIT, *PKPH_DYN_INIT;

typedef union _KPH_DYN_ATOMIC
{
    PKPH_DYN Dyn;
    KPH_ATOMIC_OBJECT_REF Atomic;
} KPH_DYN_ATOMIC, *PKPH_DYN_ATOMIC;

typedef struct _KPH_DYN_MODULE
{
    USHORT Class;
    UNICODE_STRING Name;
    BOOLEAN Valid;
    USHORT Machine;
    ULONG TimeDateStamp;
    ULONG SizeOfImage;
} KPH_DYN_MODULE, *PKPH_DYN_MODULE;

typedef enum _KPH_DYN_MODULE_CLASS
{
    KphDynNtoskrnl,
    KphDynNtkrla57,
    KphDynLxcore,
} KPH_DYN_MODULE_CLASS, *PKPH_DYN_MODULE_CLASS;

typedef struct _KPH_DYN_HW_SPAN
{
    ULONG Offset;
    ULONG Length;
    ULONG NewOffset;
} KPH_DYN_HW_SPAN, *PKPH_DYN_HW_SPAN;
typedef const KPH_DYN_HW_SPAN* PCKPH_DYN_HW_SPAN;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
DECLARE_CONST_UNICODE_STRING(KphpDynDataTypeName, L"KphDynData");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpDynDataType = NULL;
static KPH_DYN_MODULE KphpDynModules[] =
{
    { KPH_DYN_CLASS_NTOSKRNL, RTL_CONSTANT_STRING(L"ntoskrnl.exe"), FALSE, 0, 0, 0 },
    { KPH_DYN_CLASS_NTKRLA57, RTL_CONSTANT_STRING(L"ntkrla57.exe"), FALSE, 0, 0, 0 },
    { KPH_DYN_CLASS_LXCORE,   RTL_CONSTANT_STRING(L"lxcore.sys"),   FALSE, 0, 0, 0 },
};
KPH_PROTECTED_DATA_SECTION_POP();
static KPH_DYN_ATOMIC KphpDynData = { .Atomic = KPH_ATOMIC_OBJECT_REF_INIT };

/**
 * \brief Reference the dynamic configuration.
 *
 * \return Pointer to the dynamic configuration, NULL if not activated. The
 * caller must eventually dereference the object.
 */
_IRQL_requires_max_(HIGH_LEVEL)
_Must_inspect_result_
PKPH_DYN KphReferenceDynData(
    VOID
    )
{
    KPH_NPAGED_CODE_HIGH_MAX();

    return KphAtomicReferenceObject(&KphpDynData.Atomic);
}

KPH_PAGED_FILE();

/**
 * \brief Allocates a dynamic configuration object.
 *
 * \param[in] Size The size of the object to allocate.
 *
 * \return Pointer to the allocated object, NULL on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateDynData(
    _In_ SIZE_T Size
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphAllocateNPaged(Size, KPH_TAG_DYNDATA);
}

/**
 * \brief Orders hardware section table spans by offset, then length.
 *
 * \param[in] Left The first span.
 * \param[in] Right The second span.
 *
 * \return Negative, zero, or positive ordering of the spans.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
INT KphpDynHwCompareSpan(
    _In_ PCKPH_DYN_HW_SPAN Left,
    _In_ PCKPH_DYN_HW_SPAN Right
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Left->Offset != Right->Offset)
    {
        return ((Left->Offset < Right->Offset) ? -1 : 1);
    }

    if (Left->Length != Right->Length)
    {
        return ((Left->Length < Right->Length) ? -1 : 1);
    }

    return 0;
}

/**
 * \brief Sorts hardware section table spans.
 *
 * \param[in] Context Unused.
 * \param[in] LeftElement The first span.
 * \param[in] RightElement The second span.
 *
 * \return Negative, zero, or positive ordering of the spans.
 */
_Function_class_(KPH_QUICK_SORT_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
INT KSIAPI KphpDynHwSortSpan(
    _In_opt_ PVOID Context,
    _In_ PCVOID LeftElement,
    _In_ PCVOID RightElement
    )
{
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    return KphpDynHwCompareSpan(LeftElement, RightElement);
}

/**
 * \brief Searches sorted hardware section table spans.
 *
 * \param[in] Context Unused.
 * \param[in] Key The span to find.
 * \param[in] Element The span to compare against.
 *
 * \return Negative, zero, or positive ordering of the spans.
 */
_Function_class_(KPH_BINARY_SEARCH_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
INT KSIAPI KphpDynHwSearchSpan(
    _In_opt_ PVOID Context,
    _In_ PCVOID Key,
    _In_ PCVOID Element
    )
{
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    return KphpDynHwCompareSpan(Key, Element);
}

/**
 * \brief Checks whether a hardware section row can apply to this system.
 *
 * \param[in] Index The row to check.
 * \param[in] Cpu The CPU identity of this system.
 * \param[in] Board The board identity of this system.
 *
 * \return TRUE if the row is kept, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphpDynHwKeepRow(
    _In_ PCKPH_DYN_HW_INDEX Index,
    _In_ PCKPH_DYN_HW_CPU_IDENTITY Cpu,
    _In_ PCKPH_DYN_HW_BOARD_IDENTITY Board
    )
{
    KPH_PAGED_CODE_PASSIVE();

    switch (Index->IdentityKind)
    {
        case KPH_DYN_HW_IDENTITY_CPU:
        case KPH_DYN_HW_IDENTITY_CPU_AND_PCI:
        case KPH_DYN_HW_IDENTITY_CPU_AND_PCI_CLASS:
        {
#if defined(_M_X64)
            return KphEqualMemory(&Index->Identity.Cpu,
                                  Cpu,
                                  sizeof(KPH_DYN_HW_CPU_IDENTITY));
#else
            UNREFERENCED_PARAMETER(Cpu);
            return FALSE;
#endif
        }
        case KPH_DYN_HW_IDENTITY_BOARD:
        {
            if (Index->Identity.Board.BaseboardHash &&
                (Index->Identity.Board.BaseboardHash == Board->BaseboardHash))
            {
                return TRUE;
            }

            if (Index->Identity.Board.SystemHash &&
                (Index->Identity.Board.SystemHash == Board->SystemHash))
            {
                return TRUE;
            }

            return FALSE;
        }
        case KPH_DYN_HW_IDENTITY_ARM_CPU:
        {
#if defined(_M_ARM64)
            return TRUE;
#else
            return FALSE;
#endif
        }
        default:
        {
            return TRUE;
        }
    }
}

/**
 * \brief Copies the rows of a hardware section that can apply to this system.
 *
 * \details The copy is a valid section: kept rows stay in order and kept
 * tables are packed in order with their offsets rewritten.
 *
 * \param[in] Section The verified hardware section.
 * \param[in] Cpu The CPU identity of this system.
 * \param[in] Board The board identity of this system.
 * \param[out] Filtered Receives the filtered section, freed with
 * KPH_TAG_HW_SECTION.
 * \param[out] FilteredLength Receives the length of the filtered section.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpDynHwFilterSection(
    _In_ PCKPH_DYN_HW_SECTION Section,
    _In_ PCKPH_DYN_HW_CPU_IDENTITY Cpu,
    _In_ PCKPH_DYN_HW_BOARD_IDENTITY Board,
    _Outptr_result_bytebuffer_(*FilteredLength) PKPH_DYN_HW_SECTION* Filtered,
    _Out_ PULONG FilteredLength
    )
{
    NTSTATUS status;
    PCKPH_DYN_HW_INDEX index;
    PCVOID body;
    PKPH_DYN_HW_SPAN spans;
    ULONG rowCount;
    ULONG spanCount;
    ULONG bodyLength;
    ULONG length;
    PKPH_DYN_HW_SECTION filtered;
    PKPH_DYN_HW_INDEX filteredIndex;
    PVOID filteredBody;

    KPH_PAGED_CODE_PASSIVE();

    *Filtered = NULL;
    *FilteredLength = 0;

    spans = NULL;
    filtered = NULL;

    index = Add2Ptr(Section, sizeof(KPH_DYN_HW_SECTION));
    body = &index[Section->Count];

    rowCount = 0;

    for (ULONG i = 0; i < Section->Count; i++)
    {
        if (KphpDynHwKeepRow(&index[i], Cpu, Board))
        {
            rowCount++;
        }
    }

    if (rowCount)
    {
        spans = KphAllocatePaged(sizeof(KPH_DYN_HW_SPAN) * rowCount,
                                 KPH_TAG_HW_SECTION_SPANS);
        if (!spans)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to allocate hardware section spans");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }
    }

    spanCount = 0;

    for (ULONG i = 0; i < Section->Count; i++)
    {
        if (KphpDynHwKeepRow(&index[i], Cpu, Board))
        {
            spans[spanCount].Offset = index[i].Offset;
            spans[spanCount].Length = index[i].Length;
            spans[spanCount].NewOffset = 0;
            spanCount++;
        }
    }

    NT_ASSERT(spanCount == rowCount);

    if (spanCount)
    {
        ULONG unique;

        KphQuickSort(spans,
                     spanCount,
                     sizeof(KPH_DYN_HW_SPAN),
                     KphpDynHwSortSpan,
                     NULL);

        unique = 1;

        for (ULONG i = 1; i < spanCount; i++)
        {
            if (KphpDynHwCompareSpan(&spans[i], &spans[unique - 1]))
            {
                spans[unique++] = spans[i];
            }
        }

        spanCount = unique;
    }

    //
    // Every kept span is within the verified section, so the packed copy is
    // never longer than the original.
    //
    bodyLength = 0;

    for (ULONG i = 0; i < spanCount; i++)
    {
        spans[i].NewOffset = bodyLength;

        NT_VERIFY(NT_SUCCESS(RtlULongAdd(bodyLength,
                                         spans[i].Length,
                                         &bodyLength)));
    }

    NT_VERIFY(NT_SUCCESS(RtlULongMult(sizeof(KPH_DYN_HW_INDEX),
                                      rowCount,
                                      &length)));
    NT_VERIFY(NT_SUCCESS(RtlULongAdd(length,
                                     sizeof(KPH_DYN_HW_SECTION),
                                     &length)));
    NT_VERIFY(NT_SUCCESS(RtlULongAdd(length, bodyLength, &length)));

    NT_ASSERT(length <= Section->Length);

    filtered = KphAllocatePaged(length, KPH_TAG_HW_SECTION);
    if (!filtered)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate hardware section");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    filtered->Magic = Section->Magic;
    filtered->Length = length;
    filtered->Count = rowCount;
    filtered->Reserved = 0;

    filteredIndex = Add2Ptr(filtered, sizeof(KPH_DYN_HW_SECTION));
    filteredBody = &filteredIndex[rowCount];

    rowCount = 0;

    for (ULONG i = 0; i < Section->Count; i++)
    {
        KPH_DYN_HW_SPAN key;
        PCKPH_DYN_HW_SPAN span;

        if (!KphpDynHwKeepRow(&index[i], Cpu, Board))
        {
            continue;
        }

        key.Offset = index[i].Offset;
        key.Length = index[i].Length;
        key.NewOffset = 0;

        span = KphBinarySearch(&key,
                               spans,
                               spanCount,
                               sizeof(KPH_DYN_HW_SPAN),
                               KphpDynHwSearchSpan,
                               NULL);
        NT_ASSERT(span);
        _Analysis_assume_(span);

        filteredIndex[rowCount] = index[i];
        filteredIndex[rowCount].Offset = span->NewOffset;
        rowCount++;
    }

    for (ULONG i = 0; i < spanCount; i++)
    {
        RtlCopyMemory(Add2Ptr(filteredBody, spans[i].NewOffset),
                      Add2Ptr(body, spans[i].Offset),
                      spans[i].Length);
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "Hardware section filtered: %lu of %lu rows, "
                  "%lu of %lu bytes",
                  filtered->Count,
                  Section->Count,
                  filtered->Length,
                  Section->Length);

    *Filtered = filtered;
    *FilteredLength = length;
    filtered = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (filtered)
    {
        KphFree(filtered, KPH_TAG_HW_SECTION);
    }

    if (spans)
    {
        KphFree(spans, KPH_TAG_HW_SECTION_SPANS);
    }

    return status;
}

/**
 * \brief Copies this system's hardware section rows and resolves this CPU's
 * and board's rows.
 *
 * \param[in,out] Dyn Dynamic configuration object.
 * \param[in] Init Initialization parameters for dynamic configuration.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpInitializeDynHardware(
    _Inout_ PKPH_DYN Dyn,
    _In_ PKPH_DYN_INIT Init
    )
{
    NTSTATUS status;
    KPH_DYN_HW_IDENTITY identity;
    KPH_DYN_HW_CPU_IDENTITY cpuIdentity;
    PCKPH_DYN_HW_BOARD_IDENTITY board;
#if defined(_M_X64)
    PCKPH_HW_CPU_IDENTITY cpu;
#endif

    KPH_PAGED_CODE_PASSIVE();

    if (!Init->Hardware)
    {
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(&cpuIdentity, sizeof(cpuIdentity));

#if defined(_M_X64)
    cpu = KphGetHardwareCpuIdentity();

    RtlCopyMemory(cpuIdentity.Vendor,
                  cpu->Vendor,
                  sizeof(cpuIdentity.Vendor));
    cpuIdentity.DisplayFamily = cpu->DisplayFamily;
    cpuIdentity.DisplayModel = cpu->DisplayModel;
#endif

    board = KphGetHardwareBoardIdentity();

    status = KphpDynHwFilterSection(Init->Hardware,
                                    &cpuIdentity,
                                    board,
                                    &Dyn->Hardware.Section,
                                    &Dyn->Hardware.SectionLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpDynHwFilterSection failed: %!STATUS!",
                      status);

        return status;
    }

#if defined(_M_X64)
    RtlZeroMemory(&identity, sizeof(identity));
    identity.Cpu = cpuIdentity;

    status = KphDynHwFindTable(Dyn->Hardware.Section,
                               KPH_DYN_HW_IDENTITY_CPU,
                               &identity,
                               &Dyn->Hardware.CpuTable);
    NT_VERIFY(NT_SUCCESS(status) || (status == STATUS_NOT_FOUND));
#endif

    if (board->BaseboardHash)
    {
        RtlZeroMemory(&identity, sizeof(identity));
        identity.Board.BaseboardHash = board->BaseboardHash;

        status = KphDynHwFindTable(Dyn->Hardware.Section,
                                   KPH_DYN_HW_IDENTITY_BOARD,
                                   &identity,
                                   &Dyn->Hardware.BoardTable);
        NT_VERIFY(NT_SUCCESS(status) || (status == STATUS_NOT_FOUND));
    }

    if (!Dyn->Hardware.BoardTable && board->SystemHash)
    {
        RtlZeroMemory(&identity, sizeof(identity));
        identity.Board.SystemHash = board->SystemHash;

        status = KphDynHwFindTable(Dyn->Hardware.Section,
                                   KPH_DYN_HW_IDENTITY_BOARD,
                                   &identity,
                                   &Dyn->Hardware.BoardTable);
        NT_VERIFY(NT_SUCCESS(status) || (status == STATUS_NOT_FOUND));

        Dyn->Hardware.BoardSystem = !!Dyn->Hardware.BoardTable;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "Dynamic Configuration hardware section: "
                  "%lu bytes, %lu rows, CPU row %!bool!, board row %!bool!, "
                  "board system %!bool!",
                  Dyn->Hardware.SectionLength,
                  Dyn->Hardware.Section->Count,
                  !!Dyn->Hardware.CpuTable,
                  !!Dyn->Hardware.BoardTable,
                  Dyn->Hardware.BoardSystem);

    return STATUS_SUCCESS;
}

/**
 * \brief Initializes a dynamic configuration object.
 *
 * \param[in] Object Dynamic configuration object to initialize.
 * \param[in] Parameter Initialization parameters for dynamic configuration.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializeDynData(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    PKPH_DYN_INIT init;
    KPH_VERIFY_KEY_HANDLE sessionTokenPublicKeyHandle;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

    dyn = Object;
    init = Parameter;

#define KPH_LOAD_DYNITEM_KERNEL(x)                                             \
    dyn->x = init->Kernel ? C_2sTo4(init->Kernel->x) : ULONG_MAX;
#define KPH_LOAD_DYNITEM_LXCORE(x)                                             \
    dyn->x = init->Lxcore ? C_2sTo4(init->Lxcore->x) : ULONG_MAX;

    KPH_LOAD_DYNITEM_KERNEL(EgeGuid);
    KPH_LOAD_DYNITEM_KERNEL(EpObjectTable);
    KPH_LOAD_DYNITEM_KERNEL(EreGuidEntry);
    KPH_LOAD_DYNITEM_KERNEL(HtHandleContentionEvent);
    KPH_LOAD_DYNITEM_KERNEL(OtName);
    KPH_LOAD_DYNITEM_KERNEL(OtIndex);
    KPH_LOAD_DYNITEM_KERNEL(ObDecodeShift);
    KPH_LOAD_DYNITEM_KERNEL(ObAttributesShift);
    KPH_LOAD_DYNITEM_KERNEL(AlpcCommunicationInfo);
    KPH_LOAD_DYNITEM_KERNEL(AlpcOwnerProcess);
    KPH_LOAD_DYNITEM_KERNEL(AlpcConnectionPort);
    KPH_LOAD_DYNITEM_KERNEL(AlpcServerCommunicationPort);
    KPH_LOAD_DYNITEM_KERNEL(AlpcClientCommunicationPort);
    KPH_LOAD_DYNITEM_KERNEL(AlpcHandleTable);
    KPH_LOAD_DYNITEM_KERNEL(AlpcHandleTableLock);
    KPH_LOAD_DYNITEM_KERNEL(AlpcAttributes);
    KPH_LOAD_DYNITEM_KERNEL(AlpcAttributesFlags);
    KPH_LOAD_DYNITEM_KERNEL(AlpcPortContext);
    KPH_LOAD_DYNITEM_KERNEL(AlpcPortObjectLock);
    KPH_LOAD_DYNITEM_KERNEL(AlpcSequenceNo);
    KPH_LOAD_DYNITEM_KERNEL(AlpcState);
    KPH_LOAD_DYNITEM_KERNEL(AlpcBlobSize);
    KPH_LOAD_DYNITEM_KERNEL(AlpcBlobLock);
    KPH_LOAD_DYNITEM_KERNEL(KtInitialStack);
    KPH_LOAD_DYNITEM_KERNEL(KtStackLimit);
    KPH_LOAD_DYNITEM_KERNEL(KtStackBase);
    KPH_LOAD_DYNITEM_KERNEL(KtKernelStack);
    KPH_LOAD_DYNITEM_KERNEL(KtReadOperationCount);
    KPH_LOAD_DYNITEM_KERNEL(KtWriteOperationCount);
    KPH_LOAD_DYNITEM_KERNEL(KtOtherOperationCount);
    KPH_LOAD_DYNITEM_KERNEL(KtReadTransferCount);
    KPH_LOAD_DYNITEM_KERNEL(KtWriteTransferCount);
    KPH_LOAD_DYNITEM_KERNEL(KtOtherTransferCount);
    KPH_LOAD_DYNITEM_KERNEL(MmSectionControlArea);
    KPH_LOAD_DYNITEM_KERNEL(MmControlAreaListHead);
    KPH_LOAD_DYNITEM_KERNEL(MmControlAreaLock);
    KPH_LOAD_DYNITEM_KERNEL(EpSectionObject);

    KPH_LOAD_DYNITEM_LXCORE(LxPicoProc);
    KPH_LOAD_DYNITEM_LXCORE(LxPicoProcInfo);
    KPH_LOAD_DYNITEM_LXCORE(LxPicoProcInfoPID);
    KPH_LOAD_DYNITEM_LXCORE(LxPicoThrdInfo);
    KPH_LOAD_DYNITEM_LXCORE(LxPicoThrdInfoTID);

    status = KphVerifyCreateKey(&sessionTokenPublicKeyHandle,
                                init->Config->SessionTokenPublicKey,
                                KPH_DYN_SESSION_TOKEN_PUBLIC_KEY_LENGTH);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphVerifyCreateKey failed: %!STATUS!",
                      status);

        sessionTokenPublicKeyHandle = NULL;
        goto Exit;
    }

    status = KphpInitializeDynHardware(dyn, init);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpInitializeDynHardware failed: %!STATUS!",
                      status);

        goto Exit;
    }

    dyn->SessionTokenPublicKeyHandle = sessionTokenPublicKeyHandle;
    sessionTokenPublicKeyHandle = NULL;

    if (init->Kernel)
    {
        SetFlag(dyn->ActiveFlags, KPH_DYN_ACTIVE_KERNEL);
    }

    if (init->Lxcore)
    {
        SetFlag(dyn->ActiveFlags, KPH_DYN_ACTIVE_LXCORE);
    }

    if (dyn->Hardware.Section)
    {
        SetFlag(dyn->ActiveFlags, KPH_DYN_ACTIVE_HARDWARE);
    }

Exit:

    if (sessionTokenPublicKeyHandle)
    {
        KphVerifyCloseKey(sessionTokenPublicKeyHandle);
    }

    return status;
}

/**
 * \brief Deletes a dynamic configuration object.
 *
 * \param[in] Object Dynamic configuration object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpDeleteDynData(
    _In_freesMem_ PVOID Object
    )
{
    PKPH_DYN dyn;

    KPH_PAGED_CODE_PASSIVE();

    dyn = Object;

    if (dyn->SessionTokenPublicKeyHandle)
    {
        KphVerifyCloseKey(dyn->SessionTokenPublicKeyHandle);
    }

    if (dyn->Hardware.Section)
    {
        KphFree(dyn->Hardware.Section, KPH_TAG_HW_SECTION);
    }
}

/**
 * \brief Frees a dynamic configuration object.
 *
 * \param[in] Object Dynamic configuration object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpFreeDynData(
    _In_freesMem_ PVOID Object
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphFree(Object, KPH_TAG_DYNDATA);
}

/**
 * \brief Activates dynamic data.
 *
 * \param[in] DynConfig The dynamic configuration to activate.
 * \param[in] DynConfigLength The length of the dynamic configuration.
 * \param[in] Signature The signature of the dynamic data.
 * \param[in] SignatureLength The length of the signature.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpActivateDynData(
    _In_ PBYTE DynConfig,
    _In_ ULONG DynConfigLength,
    _In_opt_ PBYTE Signature,
    _In_ ULONG SignatureLength
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    KPH_DYN_INIT init;
    PKPH_DYN_KERNEL_FIELDS kernel;
    PKPH_DYN_LXCORE_FIELDS lxcore;
    PKPH_DYN_HW_SECTION hardwareSection;
    ULONG hardwareSectionLength;

    KPH_PAGED_CODE_PASSIVE();

    dyn = NULL;
    kernel = NULL;
    lxcore = NULL;

    if (Signature)
    {
        status = KphVerifyBuffer(DynConfig,
                                 DynConfigLength,
                                 Signature,
                                 SignatureLength);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "KphVerifyBuffer failed: %!STATUS!",
                          status);

            status = STATUS_SI_DYNDATA_INVALID_SIGNATURE;
            goto Exit;
        }
    }

    if (KphpDynModules[KphDynNtoskrnl].Valid)
    {
        status = KphDynDataLookup((PKPH_DYN_CONFIG)DynConfig,
                                  DynConfigLength,
                                  KphpDynModules[KphDynNtoskrnl].Class,
                                  KphpDynModules[KphDynNtoskrnl].Machine,
                                  KphpDynModules[KphDynNtoskrnl].TimeDateStamp,
                                  KphpDynModules[KphDynNtoskrnl].SizeOfImage,
                                  NULL,
                                  sizeof(KPH_DYN_NTOSKRNL_FIELDS),
                                  &kernel);
    }
    else if (KphpDynModules[KphDynNtkrla57].Valid)
    {
        status = KphDynDataLookup((PKPH_DYN_CONFIG)DynConfig,
                                  DynConfigLength,
                                  KphpDynModules[KphDynNtkrla57].Class,
                                  KphpDynModules[KphDynNtkrla57].Machine,
                                  KphpDynModules[KphDynNtkrla57].TimeDateStamp,
                                  KphpDynModules[KphDynNtkrla57].SizeOfImage,
                                  NULL,
                                  sizeof(KPH_DYN_NTKRLA57_FIELDS),
                                  &kernel);
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphDynDataLookup failed: %!STATUS!",
                      status);

        kernel = NULL;
    }

    if (KphpDynModules[KphDynLxcore].Valid)
    {
        status = KphDynDataLookup((PKPH_DYN_CONFIG)DynConfig,
                                  DynConfigLength,
                                  KphpDynModules[KphDynLxcore].Class,
                                  KphpDynModules[KphDynLxcore].Machine,
                                  KphpDynModules[KphDynLxcore].TimeDateStamp,
                                  KphpDynModules[KphDynLxcore].SizeOfImage,
                                  NULL,
                                  sizeof(KPH_DYN_LXCORE_FIELDS),
                                  &lxcore);
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphDynDataLookup failed: %!STATUS!",
                      status);

        lxcore = NULL;
    }

    status = KphDynDataLookupHardware((PKPH_DYN_CONFIG)DynConfig,
                                      DynConfigLength,
                                      &hardwareSection,
                                      &hardwareSectionLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynDataLookupHardware failed: %!STATUS!",
                      status);

        hardwareSection = NULL;
        hardwareSectionLength = 0;
    }

    init.Config = (PKPH_DYN_CONFIG)DynConfig;
    init.Kernel = kernel;
    init.Lxcore = lxcore;
    init.Hardware = hardwareSection;
    init.HardwareLength = hardwareSectionLength;

    status = KphCreateObject(KphpDynDataType, sizeof(KPH_DYN), &dyn, &init);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphAtomicAssignObjectReference(&KphpDynData.Atomic, dyn);

    KphTracePrint(TRACE_LEVEL_INFORMATION,
                  GENERAL,
                  "Activated Dynamic Configuration "
                  "for Windows %lu.%lu.%lu Kernel %hu.%hu.%hu.%hu "
                  "(flags 0x%08lx)",
                  KphOsVersionInfo.dwMajorVersion,
                  KphOsVersionInfo.dwMinorVersion,
                  KphOsVersionInfo.dwBuildNumber,
                  KphKernelVersion.MajorVersion,
                  KphKernelVersion.MinorVersion,
                  KphKernelVersion.BuildNumber,
                  KphKernelVersion.Revision,
                  dyn->ActiveFlags);

    status = STATUS_SUCCESS;

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
}

/**
 * \brief Activates dynamic data.
 *
 * \param[in] DynData The dynamic data to activate.
 * \param[in] DynDataLength The length of the dynamic data.
 * \param[in] Signature The signature of the dynamic data.
 * \param[in] SignatureLength The length of the signature.
 * \param[out] Flags Receives the parts of the active configuration, whether
 * or not this one was activated.
 * \param[in] AccessMode The access mode of the caller.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphActivateDynData(
    _In_ PBYTE DynData,
    _In_ ULONG DynDataLength,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength,
    _Out_opt_ PKPH_DYN_ACTIVE_FLAGS Flags,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PBYTE dynData;
    PBYTE signature;

    KPH_PAGED_CODE_PASSIVE();

    dynData = NULL;
    signature = NULL;

    if (!DynData || !DynDataLength)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!Signature || !SignatureLength)
    {
        status = STATUS_SI_DYNDATA_INVALID_SIGNATURE;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        dynData = KphAllocatePaged(DynDataLength, KPH_TAG_DYNDATA);
        if (!dynData)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        signature = KphAllocatePaged(SignatureLength, KPH_TAG_DYNDATA);
        if (!signature)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        __try
        {
            CopyFromUser(dynData, DynData, DynDataLength);
            CopyFromUser(signature, Signature, SignatureLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        dynData = DynData;
        signature = Signature;
    }

    status = KphpActivateDynData(dynData,
                                 DynDataLength,
                                 signature,
                                 SignatureLength);

Exit:

    if (Flags)
    {
        KphWriteULongToMode((PULONG)Flags,
                            KphQueryActiveDynData(),
                            AccessMode);
    }

    if (dynData && (dynData != DynData))
    {
        KphFree(dynData, KPH_TAG_DYNDATA);
    }

    if (signature && (signature != Signature))
    {
        KphFree(signature, KPH_TAG_DYNDATA);
    }

    return status;
}

/**
 * \brief Retrieves the parts of the active dynamic configuration.
 *
 * \return The active parts, KPH_DYN_ACTIVE_NONE when none is active.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
KPH_DYN_ACTIVE_FLAGS KphQueryActiveDynData(
    VOID
    )
{
    PKPH_DYN dyn;
    KPH_DYN_ACTIVE_FLAGS flags;

    KPH_PAGED_CODE_PASSIVE();

    dyn = KphReferenceDynData();
    if (!dyn)
    {
        return KPH_DYN_ACTIVE_NONE;
    }

    flags = dyn->ActiveFlags;

    KphDereferenceObject(dyn);

    return flags;
}

/**
 * \brief Initializes the dynamic modules.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpInitializeDynModules(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KeEnterCriticalRegion();
    if (!ExAcquireResourceSharedLite(PsLoadedModuleResource, TRUE))
    {
        KeLeaveCriticalRegion();

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Failed to acquire PsLoadedModuleResource");

        return;
    }

    for (ULONG i = 0; i < ARRAYSIZE(KphpDynModules); i++)
    {
        PKPH_DYN_MODULE dynModule;

        dynModule = &KphpDynModules[i];

        for (PLIST_ENTRY link = PsLoadedModuleList->Flink;
             link != PsLoadedModuleList;
             link = link->Flink)
        {
            NTSTATUS status;
            PKLDR_DATA_TABLE_ENTRY entry;
            KPH_IMAGE_NT_HEADERS image;

            entry = CONTAINING_RECORD(link, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

            if (!RtlEqualUnicodeString(&entry->BaseDllName, &dynModule->Name, TRUE))
            {
                continue;
            }

            __try
            {
                status = KphImageNtHeader(entry->DllBase, entry->SizeOfImage, &image);
                if (!NT_SUCCESS(status))
                {
                    KphTracePrint(TRACE_LEVEL_VERBOSE,
                                  GENERAL,
                                  "KphImageNtHeader failed: %!STATUS!",
                                  status);

                    break;
                }

                dynModule->Machine = image.Headers->FileHeader.Machine;
                dynModule->TimeDateStamp = image.Headers->FileHeader.TimeDateStamp;
                if (RTL_CONTAINS_FIELD(&image.Headers->OptionalHeader,
                                       image.Headers->FileHeader.SizeOfOptionalHeader,
                                       SizeOfImage))
                {
                    dynModule->SizeOfImage = image.Headers->OptionalHeader.SizeOfImage;
                    dynModule->Valid = TRUE;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "Failed to read module headers: %!STATUS!",
                              GetExceptionCode());
            }

            break;
        }
    }

    ExReleaseResourceLite(PsLoadedModuleResource);
    KeLeaveCriticalRegion();
}

/**
 * \brief Initializes the dynamic data infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeDynData(
    VOID
    )
{
    KPH_OBJECT_TYPE_INFO typeInfo;

    KPH_PAGED_CODE_PASSIVE();

    KphpInitializeDynModules();

    typeInfo.Allocate = KphpAllocateDynData;
    typeInfo.Initialize = KphpInitializeDynData;
    typeInfo.Delete = KphpDeleteDynData;
    typeInfo.Free = KphpFreeDynData;
    typeInfo.Flags = 0;
    typeInfo.DeferDelete = TRUE;

    KphCreateObjectType(&KphpDynDataTypeName, &typeInfo, &KphpDynDataType);
}

/**
 * \brief Cleans up the dynamic data infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupDynData(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphAtomicAssignObjectReference(&KphpDynData.Atomic, NULL);
}

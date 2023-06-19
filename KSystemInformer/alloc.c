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
#include <dyndata.h>

#include <trace.h>

static PKPH_OBJECT_TYPE KphpPagedLookasideObjectType = NULL;
static UNICODE_STRING KphpPagedLookasideObjectTypeName = RTL_CONSTANT_STRING(L"KphPagedLookaside");

static PKPH_OBJECT_TYPE KphpNPagedLookasideObjectType = NULL;
static UNICODE_STRING KphpNPagedLookasideObjectTypeName = RTL_CONSTANT_STRING(L"KphNPagedLookaside");

typedef struct _KPH_LOOKASIDE_INIT
{
    SIZE_T Size;
    ULONG Tag;
} KPH_LOOKASIDE_INIT, *PKPH_LOOKASIDE_INIT;

/**
 * \brief Allocates non-page-able memory.
 *
 * \param[in] NumberOfBytes The number of bytes to allocate.
 * \param[in] Tag The pool tag to use.
 *
 * \return Allocated non-page-able memory. Null on failure.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_size_(NumberOfBytes)
PVOID KphAllocateNPaged(
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    return ExAllocatePoolZero(NonPagedPoolNx, NumberOfBytes, Tag);
}

/**
 * \brief Frees memory.
 *
 * \param[in] Memory The memory to free.
 * \param[in] Tag The tag associated with the allocation.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphFree(
    _FreesMem_ PVOID Memory,
    _In_ ULONG Tag
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    NT_ASSERT(Memory);

    ExFreePoolWithTag(Memory, Tag);
}

/**
 * \brief Initialized a non-page-able lookaside list.
 *
 * \param[out] Lookaside The lookaside list to initialize.
 * \param[in] Size The size of entries in the lookaside list.
 * \param[in] Tag The pool tag to use.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphInitializeNPagedLookaside(
    _Out_ PNPAGED_LOOKASIDE_LIST Lookaside,
    _In_ SIZE_T Size,
    _In_ ULONG Tag
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ExInitializeNPagedLookasideList(Lookaside,
                                    NULL,
                                    NULL,
                                    POOL_NX_ALLOCATION,
                                    Size,
                                    Tag,
                                    0);
}

/**
 * \brief Deletes a non-page-able lookaside list.
 *
 * \param[in,out] Lookaside The lookaside to delete.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphDeleteNPagedLookaside(
    _Inout_ PNPAGED_LOOKASIDE_LIST Lookaside
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ExDeleteNPagedLookasideList(Lookaside);
}

/**
 * \brief Allocates non-page-able memory from the lookaside list.
 *
 * \param[in,out] Lookaside The lookaside list to allocate from.
 *
 * \return Allocated non-page-able memory. Null on failure.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_size_(Lookaside->L.Size)
PVOID KphAllocateFromNPagedLookaside(
    _Inout_ PNPAGED_LOOKASIDE_LIST Lookaside
    )
{
    PVOID memory;

    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    memory = ExAllocateFromNPagedLookasideList(Lookaside);
    if (memory)
    {
        RtlZeroMemory(memory, Lookaside->L.Size);
    }

    return memory;
}

/**
 * \brief Frees non-page-able memory back to the lookaside list.
 *
 * \param[in,out] Lookaside The lookaside list to free back to.
 * \param[in] Memory The memory to free.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphFreeToNPagedLookaside(
    _Inout_ PNPAGED_LOOKASIDE_LIST Lookaside,
    _In_freesMem_ PVOID Memory
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    NT_ASSERT(Memory);

    ExFreeToNPagedLookasideList(Lookaside, Memory);
}

/**
 * \brief Allocates a non-page-able lookaside object.
 *
 * \param[in] Size The size of the lookaside object.
 *
 * \return Allocated lookaside object, null on allocation failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateNPagedLookasideObject(
    _In_ SIZE_T Size
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    return KphAllocateNPaged(Size, KPH_TAG_NPAGED_LOOKASIDE_OBJECT);
}

/**
 * \brief Initializes a non-page-able lookaside object.
 *
 * \param[in,out] Object The lookaside object to initialize.
 * \param[in] Parameter The lookaside initialization parameters.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializeNPagedLookasideObject(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_NPAGED_LOOKASIDE_OBJECT lookaside;
    PKPH_LOOKASIDE_INIT init;

    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    NT_ASSERT(Parameter);

    lookaside = Object;
    init = Parameter;

    KphInitializeNPagedLookaside((PNPAGED_LOOKASIDE_LIST)lookaside,
                                 init->Size,
                                 init->Tag);

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a non-page-able lookaside object.
 *
 * \param[in,out] Object The lookaside object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
VOID KSIAPI KphpDeleteNPagedLookasideObject(
    _Inout_ PVOID Object
    )
{
    PKPH_NPAGED_LOOKASIDE_OBJECT lookaside;

    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    lookaside = Object;

    KphDeleteNPagedLookaside((PNPAGED_LOOKASIDE_LIST)Object);
}

/**
 * \brief Frees a non-page-page lookaside object.
 *
 * \param[in] Object The lookaside object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
VOID KSIAPI KphpFreeNPagedLookasideObject(
    _In_freesMem_ PVOID Object
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    KphFree(Object, KPH_TAG_NPAGED_LOOKASIDE_OBJECT);
}

/**
 * \brief Creates a non-page-able lookaside object.
 *
 * \param[out] Lookaside Set to the new non-page-able lookaside object on success.
 * \param[in] Size The size of entries in the lookaside list.
 * \param[in] Tag The pool tag to use.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS KphCreateNPagedLookasideObject(
    _Outptr_result_nullonfailure_ PKPH_NPAGED_LOOKASIDE_OBJECT* Lookaside,
    _In_ SIZE_T Size,
    _In_ ULONG Tag
    )
{
    NTSTATUS status;
    KPH_LOOKASIDE_INIT init;

    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    init.Size = Size;
    init.Tag = Tag;

    *Lookaside = NULL;

    status = KphCreateObject(KphpNPagedLookasideObjectType,
                             sizeof(KPH_NPAGED_LOOKASIDE_OBJECT),
                             Lookaside,
                             &init);
    if (!NT_SUCCESS(status))
    {
        *Lookaside = NULL;
    }

    return status;
}

/**
 * \brief Allocates from a non-page-able lookaside object.
 *
 * \param[in,out] Lookaside The non-page-able lookaside object to allocate from.
 *
 * \return Allocated non-page-able memory. Null on failure.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_size_(Lookaside->L.Size)
PVOID KphAllocateFromNPagedLookasideObject(
    _Inout_ PKPH_NPAGED_LOOKASIDE_OBJECT Lookaside
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    return KphAllocateFromNPagedLookaside((PNPAGED_LOOKASIDE_LIST)Lookaside);
}

/**
 * \brief Frees non-page-able memory back to the lookaside object.
 *
 * \param[in,out] Lookaside The lookaside object to free back to.
 * \param[in] Memory The memory to free.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphFreeToNPagedLookasideObject(
    _Inout_ PKPH_NPAGED_LOOKASIDE_OBJECT Lookaside,
    _In_freesMem_ PVOID Memory
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    KphFreeToNPagedLookaside((PNPAGED_LOOKASIDE_LIST)Lookaside, Memory);
}


PAGED_FILE();

/**
 * \brief Allocates page-able memory.
 *
 * \param[in] NumberOfBytes The number of bytes to allocate.
 * \param[in] Tag The pool tag to use.
 *
 * \return Allocated page-able memory. Null on failure.
 */
_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_size_(NumberOfBytes)
PVOID KphAllocatePaged(
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag
    )
{
    PAGED_CODE();

    return ExAllocatePoolZero(PagedPool, NumberOfBytes, Tag);
}

/**
 * \brief Initialized a page-able lookaside list.
 *
 * \param[in] Lookaside The lookaside list to initialize.
 * \param[in] Size The size of entries in the lookaside list.
 * \param[in] Tag The pool tag to use.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphInitializePagedLookaside(
    _Out_ PPAGED_LOOKASIDE_LIST Lookaside,
    _In_ SIZE_T Size,
    _In_ ULONG Tag
    )
{
    PAGED_CODE();

    ExInitializePagedLookasideList(Lookaside, NULL, NULL, 0, Size, Tag, 0);
}

/**
 * \brief Deletes a page-able lookaside list.
 *
 * \param[in,out] Lookaside The lookaside to delete.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphDeletePagedLookaside(
    _Inout_ PPAGED_LOOKASIDE_LIST Lookaside
    )
{
    PAGED_CODE();

    ExDeletePagedLookasideList(Lookaside);
}

/**
 * \brief Allocates page-able memory from the lookaside list.
 *
 * \param[in,out] Lookaside The lookaside list to allocate from.
 *
 * \return Allocated page-able memory. Null on failure.
 */
_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_size_(Lookaside->L.Size)
PVOID KphAllocateFromPagedLookaside(
    _Inout_ PPAGED_LOOKASIDE_LIST Lookaside
    )
{
    PVOID memory;

    PAGED_CODE();

    memory = ExAllocateFromPagedLookasideList(Lookaside);
    if (memory)
    {
        RtlZeroMemory(memory, Lookaside->L.Size);
    }

    return memory;
}

/**
 * \brief Frees page-able memory back to the lookaside list.
 *
 * \param[in,out] Lookaside The lookaside list to free back to.
 * \param[in] Memory The memory to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeToPagedLookaside(
    _Inout_ PPAGED_LOOKASIDE_LIST Lookaside,
    _In_freesMem_ PVOID Memory
    )
{
    PAGED_CODE();

    NT_ASSERT(Memory);

    ExFreeToPagedLookasideList(Lookaside, Memory);
}

/**
 * \brief Allocates a page-able lookaside object.
 *
 * \param[in] Size The size of the lookaside object.
 *
 * \return Allocated lookaside object, null on allocation failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocatePagedLookasideObject(
    _In_ SIZE_T Size
    )
{
    PAGED_CODE();

    return KphAllocateNPaged(Size, KPH_TAG_PAGED_LOOKASIDE_OBJECT);
}

/**
 * \brief Initializes a page-able lookaside object.
 *
 * \param[in,out] Object The lookaside object to initialize.
 * \param[in] Parameter The lookaside initialization parameters.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializePagedLookasideObject(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_PAGED_LOOKASIDE_OBJECT lookaside;
    PKPH_LOOKASIDE_INIT init;

    PAGED_CODE();

    NT_ASSERT(Parameter);

    lookaside = Object;
    init = Parameter;

    KphInitializePagedLookaside((PPAGED_LOOKASIDE_LIST)lookaside,
                                init->Size,
                                init->Tag);

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a page-able lookaside object.
 *
 * \param[in,out] Object The lookaside object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
VOID KSIAPI KphpDeletePagedLookasideObject(
    _Inout_ PVOID Object
    )
{
    PKPH_PAGED_LOOKASIDE_OBJECT lookaside;

    PAGED_CODE();

    lookaside = Object;

    KphDeletePagedLookaside((PPAGED_LOOKASIDE_LIST)Object);
}

/**
 * \brief Frees a page-page lookaside object.
 *
 * \param[in] Object The lookaside object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
VOID KSIAPI KphpFreePagedLookasideObject(
    _In_freesMem_ PVOID Object
    )
{
    PAGED_CODE();

    KphFree(Object, KPH_TAG_PAGED_LOOKASIDE_OBJECT);
}

/**
 * \brief Creates a page-able lookaside object.
 *
 * \param[out] Lookaside Set to the new page-able lookaside object on success.
 * \param[in] Size The size of entries in the lookaside list.
 * \param[in] Tag The pool tag to use.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphCreatePagedLookasideObject(
    _Outptr_result_nullonfailure_ PKPH_PAGED_LOOKASIDE_OBJECT* Lookaside,
    _In_ SIZE_T Size,
    _In_ ULONG Tag
    )
{
    NTSTATUS status;
    KPH_LOOKASIDE_INIT init;

    PAGED_CODE();

    init.Size = Size;
    init.Tag = Tag;

    *Lookaside = NULL;

    status = KphCreateObject(KphpPagedLookasideObjectType,
                             sizeof(KPH_PAGED_LOOKASIDE_OBJECT),
                             Lookaside,
                             &init);
    if (!NT_SUCCESS(status))
    {
        *Lookaside = NULL;
    }

    return status;
}

/**
 * \brief Allocates from a page-able lookaside object.
 *
 * \param[in,out] Lookaside The page-able lookaside object to allocate from.
 *
 * \return Allocated page-able memory. Null on failure.
 */
_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_size_(Lookaside->L.Size)
PVOID KphAllocateFromPagedLookasideObject(
    _Inout_ PKPH_PAGED_LOOKASIDE_OBJECT Lookaside
    )
{
    PAGED_CODE();

    return KphAllocateFromPagedLookaside((PPAGED_LOOKASIDE_LIST)Lookaside);
}

/**
 * \brief Frees page-able memory back to the lookaside object.
 *
 * \param[in,out] Lookaside The lookaside object to free back to.
 * \param[in] Memory The memory to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeToPagedLookasideObject(
    _Inout_ PKPH_PAGED_LOOKASIDE_OBJECT Lookaside,
    _In_freesMem_ PVOID Memory
    )
{
    PAGED_CODE();

    KphFreeToPagedLookaside((PPAGED_LOOKASIDE_LIST)Lookaside, Memory);
}

/**
 * \brief Initializes allocation infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeAlloc(
    VOID
    )
{
    KPH_OBJECT_TYPE_INFO typeInfo;

    PAGED_PASSIVE();

    typeInfo.Allocate = KphpAllocatePagedLookasideObject;
    typeInfo.Initialize = KphpInitializePagedLookasideObject;
    typeInfo.Delete = KphpDeletePagedLookasideObject;
    typeInfo.Free = KphpFreePagedLookasideObject;

    KphCreateObjectType(&KphpPagedLookasideObjectTypeName,
                        &typeInfo,
                        &KphpPagedLookasideObjectType);

    typeInfo.Allocate = KphpAllocateNPagedLookasideObject;
    typeInfo.Initialize = KphpInitializeNPagedLookasideObject;
    typeInfo.Delete = KphpDeleteNPagedLookasideObject;
    typeInfo.Free = KphpFreeNPagedLookasideObject;

    KphCreateObjectType(&KphpNPagedLookasideObjectTypeName,
                        &typeInfo,
                        &KphpNPagedLookasideObjectType);
}

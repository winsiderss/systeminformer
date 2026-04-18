/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2026
 *
 */

#include <kph.h>

#include <trace.h>

#ifdef _WIN64
#define KPH_ATOMIC_OBJECT_REF_SHARED_MAX    7
#define KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_BIT 3
#define KPH_ATOMIC_OBJECT_REF_LOCK_MASK     0x000000000000000f
#define KPH_ATOMIC_OBJECT_REF_OBJECT_MASK   0xfffffffffffffff0
#else
#define KPH_ATOMIC_OBJECT_REF_SHARED_MAX    3
#define KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_BIT 2
#define KPH_ATOMIC_OBJECT_REF_LOCK_MASK     0x00000007
#define KPH_ATOMIC_OBJECT_REF_OBJECT_MASK   0xfffffff8
#endif
#define KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_FLAG (1 << KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_BIT)

//
// N.B. If more object types are added the array must be expanded.
//
static KPH_OBJECT_TYPE KphpObjectTypes[15] = { 0 };
C_ASSERT(ARRAYSIZE(KphpObjectTypes) < MAXUCHAR);
static LONG KphpObjectTypeCount = 0;
static KSI_KDPC KphpDeferDeleteObjectDpc;
static KSI_WORK_QUEUE_ITEM KphpDeferDeleteObjectWorkItem;
static SLIST_HEADER KphpDeferDeleteObjectList;

/**
 * \brief Carries out the deletion of an object.
 *
 * \param[in] Header The header of an object to delete.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpObjectDelete(
    _In_freesMem_ PKPH_OBJECT_HEADER Header
    )
{
    PKPH_OBJECT_TYPE type;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    type = &KphpObjectTypes[Header->TypeIndex];

    if (type->TypeInfo.Delete)
    {
        type->TypeInfo.Delete(KphObjectHeaderToObject(Header));
    }

    type->TypeInfo.Free(Header);

    InterlockedDecrementSizeTNoFence(&type->TotalNumberOfObjects);
}

/**
 * \brief Defers the deletion of an object.
 *
 * \param[in] Header The header of an object to defer deletion of.
 */
_IRQL_requires_max_(HIGH_LEVEL)
VOID KphpObjectDeferDelete(
    _In_ PKPH_OBJECT_HEADER Header
    )
{
    KPH_NPAGED_CODE_HIGH_MAX();

    //
    // Only schedule the worker when we transition the list from empty. The
    // worker will flush the entire list on its next pass, so subsequent
    // pushes can rely on it picking up our entry.
    //
    if (InterlockedPushEntrySList(&KphpDeferDeleteObjectList,
                                  &Header->ListEntry))
    {
        return;
    }

    if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
    {
        KsiQueueWorkItem(&KphpDeferDeleteObjectWorkItem, CriticalWorkQueue);
    }
    else
    {
        KsiInsertQueueDpc(&KphpDeferDeleteObjectDpc, NULL, NULL);
    }
}

/**
 * \brief Creates an object of a given type.
 *
 * \param[in] ObjectType The type of object to create.
 * \param[in] ObjectBodySize The size of the object body to create.
 * \param[out] Object Set to the new object on success.
 * \param[in] Parameter Optional parameter passed to initialization routine.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphCreateObject(
    _In_ PKPH_OBJECT_TYPE ObjectType,
    _In_ ULONG ObjectBodySize,
    _Outptr_result_nullonfailure_ PVOID* Object,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    PKPH_OBJECT_HEADER header;
    PVOID object;
    SIZE_T total;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    *Object = NULL;

    header = ObjectType->TypeInfo.Allocate(KphAddObjectHeaderSize(ObjectBodySize));
    if (!header)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    WriteSSizeTNoFence(&header->PointerCount, 1);
    header->TypeIndex = ObjectType->Index;

    object = KphObjectHeaderToObject(header);

    if (ObjectType->TypeInfo.Initialize)
    {
        status = ObjectType->TypeInfo.Initialize(object, Parameter);
        if (!NT_SUCCESS(status))
        {
            ObjectType->TypeInfo.Free(header);
            return status;
        }
    }

    total = InterlockedIncrementSizeTNoFence(&ObjectType->TotalNumberOfObjects);

    InterlockedExchangeIfGreaterSizeT(&ObjectType->HighWaterNumberOfObjects,
                                      total);

    *Object = object;
    return STATUS_SUCCESS;
}

/**
 * \brief References an object.
 *
 * \param[in] Object The object to reference.
 */
_IRQL_requires_max_(HIGH_LEVEL)
VOID KphReferenceObject(
    _In_ PVOID Object
    )
{
    PKPH_OBJECT_HEADER header;

    KPH_NPAGED_CODE_HIGH_MAX();

    header = KphObjectToObjectHeader(Object);

    NT_VERIFY(InterlockedIncrementSSizeTNoFence(&header->PointerCount) > 0);
}

/**
 * \brief Dereferences an object.
 *
 * \param[in] Object The object to dereference.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphDereferenceObject(
    _In_ PVOID Object
    )
{
    PKPH_OBJECT_HEADER header;
    PKPH_OBJECT_TYPE type;
    SSIZE_T refCount;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    header = KphObjectToObjectHeader(Object);

    refCount = InterlockedDecrementSSizeT(&header->PointerCount);
    if (refCount > 0)
    {
        return;
    }

    NT_ASSERT(refCount == 0);

    type = &KphpObjectTypes[header->TypeIndex];

    if (type->TypeInfo.DeferDelete)
    {
        KphpObjectDeferDelete(header);
    }
    else
    {
        KphpObjectDelete(header);
    }
}

/**
 * \brief Dereferences an object and defers deletion of the object.
 *
 * \param[in] Object The object to dereference.
 */
_IRQL_requires_max_(HIGH_LEVEL)
VOID KphDereferenceObjectDeferDelete(
    _In_ PVOID Object
    )
{
    PKPH_OBJECT_HEADER header;
    SSIZE_T refCount;

    KPH_NPAGED_CODE_HIGH_MAX();

    header = KphObjectToObjectHeader(Object);

    refCount = InterlockedDecrementSSizeT(&header->PointerCount);
    if (refCount > 0)
    {
        return;
    }

    NT_ASSERT(refCount == 0);

    KphpObjectDeferDelete(header);
}

/**
 * \brief Retrieves the type from an object.
 *
 * \param[in] Object The object to get the type of.
 *
 * \return Pointer to the type of object.
 */
_IRQL_requires_max_(HIGH_LEVEL)
_Must_inspect_result_
PKPH_OBJECT_TYPE KphGetObjectType(
    _In_ PVOID Object
    )
{
    PKPH_OBJECT_HEADER header;
    UCHAR index;
    LONG count;

    KPH_NPAGED_CODE_HIGH_MAX();

    header = KphObjectToObjectHeader(Object);
    index = header->TypeIndex;
    count = ReadAcquire(&KphpObjectTypeCount);

    if (index >= count)
    {
        return NULL;
    }

    return &KphpObjectTypes[index];
}

//
// Custom locking routines follow, disable pedantic prefast locking checks.
//
#pragma prefast(push)
#pragma prefast(disable: 26165) // possibly failing to release lock
#pragma prefast(disable: 26166) // possibly failing to acquire lock

/**
 * \brief Acquires the atomic object reference lock shared.
 *
 * \param[in,out] ObjectRef The object reference to acquire the lock for.
 */
_IRQL_requires_max_(HIGH_LEVEL)
_Requires_lock_not_held_(*ObjectRef)
_Acquires_lock_(*ObjectRef)
FORCEINLINE
VOID KphpAtomicAcquireObjectLockShared(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    ULONG_PTR object;

    KPH_NPAGED_CODE_HIGH_MAX();

    object = ReadULongPtrAcquire(&ObjectRef->Object);

    for (;; YieldProcessor())
    {
        ULONG_PTR lock;
        ULONG_PTR expected;

        lock = object & KPH_ATOMIC_OBJECT_REF_LOCK_MASK;

        if (lock >= KPH_ATOMIC_OBJECT_REF_SHARED_MAX)
        {
            object = ReadULongPtrAcquire(&ObjectRef->Object);
            continue;
        }

        expected = object;

        object = InterlockedCompareExchangeULongPtr(&ObjectRef->Object,
                                                    object + 1,
                                                    expected);
        if (object == expected)
        {
            break;
        }
    }
}

/**
 * \brief Releases the atomic object reference lock shared.
 *
 * \param[in,out] ObjectRef The object reference to release the lock of.
 */
_IRQL_requires_max_(HIGH_LEVEL)
_Requires_lock_held_(*ObjectRef)
_Releases_lock_(*ObjectRef)
FORCEINLINE
VOID KphpAtomicReleaseObjectLockShared(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    ULONG_PTR object;

    KPH_NPAGED_CODE_HIGH_MAX();

    object = InterlockedDecrementULongPtr(&ObjectRef->Object);

    object = object & KPH_ATOMIC_OBJECT_REF_SHARED_MAX;

    NT_ASSERT(object < KPH_ATOMIC_OBJECT_REF_SHARED_MAX);
}

/**
 * \brief Acquires the atomic object reference lock exclusive.
 *
 * \param[in,out] ObjectRef The object reference to acquire the lock for.
 */
_IRQL_requires_max_(HIGH_LEVEL)
_Requires_lock_not_held_(*ObjectRef)
_Acquires_lock_(*ObjectRef)
FORCEINLINE
VOID KphpAtomicAcquireObjectLockExclusive(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    ULONG_PTR object;

    KPH_NPAGED_CODE_HIGH_MAX();

    object = ReadULongPtrAcquire(&ObjectRef->Object);

    for (;; YieldProcessor())
    {
        ULONG_PTR locked;
        ULONG_PTR expected;

        if (object & KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_FLAG)
        {
            object = ReadULongPtrAcquire(&ObjectRef->Object);
            continue;
        }

        locked = object | KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_FLAG;
        expected = object;

        object = InterlockedCompareExchangeULongPtr(&ObjectRef->Object,
                                                    locked,
                                                    expected);
        if (object == expected)
        {
            break;
        }
    }

    for (; object & KPH_ATOMIC_OBJECT_REF_SHARED_MAX; YieldProcessor())
    {
        object = ReadULongPtrAcquire(&ObjectRef->Object);
    }
}

/**
 * \brief Releases the atomic object reference lock shared.
 *
 * \param[in,out] ObjectRef The object reference to release the lock of.
 */
_IRQL_requires_max_(HIGH_LEVEL)
_Requires_lock_held_(*ObjectRef)
_Releases_lock_(*ObjectRef)
FORCEINLINE
VOID KphpAtomicReleaseObjectLockExclusive(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    BOOLEAN result;

    KPH_NPAGED_CODE_HIGH_MAX();

    result = InterlockedBitTestAndResetULongPtr(&ObjectRef->Object,
                                                KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_BIT);

    NT_ASSERT(result);
}

#pragma prefast(pop)
//
// End of custom locking routines.
//

/**
 * \brief Retrieves an addition object reference to an atomically managed
 * object reference.
 *
 * \details This mechanism provides a light weight and fast way to atomically
 * managed a reference to an object. If an object is currently managed an
 * additional reference to that object is acquired and must be eventually
 * released by calling KphDereferenceObject.
 *
 * \param[in] ObjectRef The object reference to retrieve the object from.
 *
 * \return A referenced object, NULL if no object is managed.
 */
_IRQL_requires_max_(HIGH_LEVEL)
_Must_inspect_result_
PVOID KphAtomicReferenceObject(
    _In_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    ULONG_PTR value;
    PVOID object;

    KPH_NPAGED_CODE_HIGH_MAX();

    KphpAtomicAcquireObjectLockShared(ObjectRef);

    value = ReadULongPtrNoFence(&ObjectRef->Object);
    object = (PVOID)(value & KPH_ATOMIC_OBJECT_REF_OBJECT_MASK);
    if (object)
    {
        KphReferenceObject(object);
    }

    KphpAtomicReleaseObjectLockShared(ObjectRef);

    return object;
}

/**
 * \brief Stores an object to an atomically managed object reference.
 *
 * \param[in,out] ObjectRef The object reference to assign the object to.
 * \param[in] Object Optional object to reference and assign.
 *
 * \return The previous object that was managed, NULL if no object was managed.
 */
_IRQL_requires_max_(HIGH_LEVEL)
_Must_inspect_result_
PVOID KphpAtomicStoreObjectReference(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef,
    _In_opt_ PVOID Object
    )
{
    PVOID previous;
    ULONG_PTR value;
    ULONG_PTR object;

    KPH_NPAGED_CODE_HIGH_MAX();

    NT_ASSERT(((ULONG_PTR)Object & KPH_ATOMIC_OBJECT_REF_LOCK_MASK) == 0);

    KphpAtomicAcquireObjectLockExclusive(ObjectRef);

    value = ReadULongPtrNoFence(&ObjectRef->Object);
    previous = (PVOID)(value & KPH_ATOMIC_OBJECT_REF_OBJECT_MASK);

    object = (ULONG_PTR)Object | KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_FLAG;

    InterlockedExchangeULongPtr(&ObjectRef->Object, object);

    KphpAtomicReleaseObjectLockExclusive(ObjectRef);

    return previous;
}

/**
 * \brief Assigns an object to an atomically managed object reference.
 *
 * \details This mechanism provides a light weight and fast way to atomically
 * managed a reference to an object. Any previously managed reference will be
 * released. If an object is provided, this function will acquire an additional
 * reference to the object, the caller should still release their reference.
 *
 * \param[in,out] ObjectRef The object reference to assign the object to.
 * \param[in] Object Optional object to reference and assign, if NULL the
 * managed reference will be cleared.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphAtomicAssignObjectReference(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef,
    _In_opt_ PVOID Object
    )
{
    PVOID previous;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    if (Object)
    {
        KphReferenceObject(Object);
    }

    previous = KphpAtomicStoreObjectReference(ObjectRef, Object);

    if (previous)
    {
        KphDereferenceObject(previous);
    }
}

/**
 * \brief Moves an object to an atomically managed object reference.
 *
 * \details This mechanism provides a light weight and fast way to atomically
 * managed a reference to an object. If any object is provided, this function
 * will assume ownership over the reference, the caller should *not* release
 * their reference. If an object is returned the ownership of it is transferred
 * to the caller, the caller is responsible for eventually releasing it.
 *
 * \param[in,out] ObjectRef The object reference to move the object into.
 * \param[in] Object Optional object move into the object reference.
 *
 * \return The previous object that was managed, NULL if no object was managed.
 */
_IRQL_requires_max_(HIGH_LEVEL)
_Must_inspect_result_
PVOID KphAtomicMoveObjectReference(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef,
    _In_opt_ PVOID Object
    )
{
    KPH_NPAGED_CODE_HIGH_MAX();

    return KphpAtomicStoreObjectReference(ObjectRef, Object);
}

/**
 * \brief DPC bridge that bounces from above-DISPATCH context down to
 * DISPATCH_LEVEL where the work-queue API can be called. Its only job is
 * to queue the deferred-delete worker.
 *
 * \param[in] Dpc Unused.
 * \param[in] DeferredContext Unused.
 * \param[in] SystemArgument1 Unused.
 * \param[in] SystemArgument2 Unused.
 */
_Function_class_(KDEFERRED_ROUTINE)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID KphpDeferDeleteObjectDpcRoutine(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )
{
    KPH_NPAGED_CODE_DISPATCH();

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    KsiQueueWorkItem(&KphpDeferDeleteObjectWorkItem, CriticalWorkQueue);
}

KPH_PAGED_FILE();

/**
 * \brief Creates an object type.
 *
 * \param[in] TypeName The name of the type, this must always be resident.
 * Preferably a string literal.
 * \param[in] TypeInfo The information for the type being created.
 * \param[out] ObjectType Set to a pointer to the object type on success.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCreateObjectType(
    _In_ PCUNICODE_STRING TypeName,
    _In_ PKPH_OBJECT_TYPE_INFO TypeInfo,
    _Outptr_ PKPH_OBJECT_TYPE* ObjectType
    )
{
    PKPH_OBJECT_TYPE type;
    LONG index;

    KPH_PAGED_CODE_PASSIVE();

    index = (InterlockedIncrement(&KphpObjectTypeCount) - 1);

    //
    // We have failure free object type creation, to achieve this we have
    // a pre-reserved sized array above. If this asserts the array wasn't
    // expanded correctly to support a new type.
    //
    NT_ASSERT((index >= 0) && (index < ARRAYSIZE(KphpObjectTypes)));
    NT_ASSERT(index < MAXUCHAR);

    type = &KphpObjectTypes[index];

    type->Name.Buffer = TypeName->Buffer;
    type->Name.MaximumLength = TypeName->MaximumLength;
    type->Name.Length = TypeName->Length;

    type->Index = (UCHAR)index;
    WriteSizeTNoFence(&type->TotalNumberOfObjects, 0);
    WriteSizeTNoFence(&type->HighWaterNumberOfObjects, 0);

    RtlCopyMemory(&type->TypeInfo, TypeInfo, sizeof(*TypeInfo));

    *ObjectType = type;
}

/**
 * \brief Worker routine for deleting objects in a deferred manner.
 *
 * \param[in] Parameter Unused parameter.
 */
_Function_class_(KSI_WORK_QUEUE_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpDeferDeleteObjectWorker(
    _In_opt_ PVOID Parameter
    )
{
    PSLIST_ENTRY entry;

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Parameter);

    entry = InterlockedFlushSList(&KphpDeferDeleteObjectList);

    while (entry)
    {
        PKPH_OBJECT_HEADER header;

        header = CONTAINING_RECORD(entry, KPH_OBJECT_HEADER, ListEntry);

        entry = entry->Next;

        KphpObjectDelete(header);
    }
}

/**
 * \brief Initializes the object subsystem.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphObjectInitialize(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    InitializeSListHead(&KphpDeferDeleteObjectList);
    KsiInitializeDpc(&KphpDeferDeleteObjectDpc,
                     KphDriverObject,
                     KphpDeferDeleteObjectDpcRoutine,
                     NULL);
    KsiInitializeWorkItem(&KphpDeferDeleteObjectWorkItem,
                          KphDriverObject,
                          KphpDeferDeleteObjectWorker,
                          NULL);
}

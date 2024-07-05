/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2023
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
static KPH_OBJECT_TYPE KphpObjectTypes[13] = { 0 };
C_ASSERT(ARRAYSIZE(KphpObjectTypes) < MAXUCHAR);
static volatile LONG KphpObjectTypeCount = 0;

/**
 * \brief Creates an object type.
 *
 * \param[in] TypeName The name of the type, this must always be resident.
 * Preferably a string literal.
 * \param[in] TypeInfo The information for the type being created.
 * \param[out] ObjectType Set to a pointer to the object type on success.
 */
VOID KphCreateObjectType(
    _In_ PCUNICODE_STRING TypeName,
    _In_ PKPH_OBJECT_TYPE_INFO TypeInfo,
    _Outptr_ PKPH_OBJECT_TYPE* ObjectType
    )
{
    PKPH_OBJECT_TYPE type;
    LONG index;

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
    type->TotalNumberOfObjects = 0;
    type->HighWaterNumberOfObjects = 0;

    RtlCopyMemory(&type->TypeInfo, TypeInfo, sizeof(*TypeInfo));

    *ObjectType = type;
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

    *Object = NULL;

    header = ObjectType->TypeInfo.Allocate(KphAddObjectHeaderSize(ObjectBodySize));
    if (!header)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    header->PointerCount = 1;
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

    total = InterlockedIncrementSizeT(&ObjectType->TotalNumberOfObjects);

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
VOID KphReferenceObject(
    _In_ PVOID Object
    )
{
    PKPH_OBJECT_HEADER header;

    header = KphObjectToObjectHeader(Object);

    NT_VERIFY(InterlockedIncrementSSizeT(&header->PointerCount) > 0);
}

/**
 * \brief Dereferences an object.
 *
 * \param[in] Object The object to dereference.
 */
VOID KphDereferenceObject(
    _In_ PVOID Object
    )
{
    PKPH_OBJECT_HEADER header;
    PKPH_OBJECT_TYPE type;
    SSIZE_T refCount;

    header = KphObjectToObjectHeader(Object);

    refCount = InterlockedDecrementSSizeT(&header->PointerCount);
    if (refCount > 0)
    {
        return;
    }

    NT_ASSERT(refCount == 0);

    type = &KphpObjectTypes[header->TypeIndex];

    if (type->TypeInfo.Delete)
    {
        type->TypeInfo.Delete(Object);
    }

    type->TypeInfo.Free(header);

    InterlockedDecrementSizeT(&type->TotalNumberOfObjects);
}

/**
 * \brief Retrieves the type from an object.
 *
 * \param[in] Object The object to get the type of.
 *
 * \return Pointer to the type of object.
 */
_Must_inspect_result_
PKPH_OBJECT_TYPE KphGetObjectType(
    _In_ PVOID Object
    )
{
    PKPH_OBJECT_HEADER header;
    UCHAR index;
    LONG count;

    header = KphObjectToObjectHeader(Object);
    index = header->TypeIndex;
    count = KphpObjectTypeCount;

    if (index >= count)
    {
        return NULL;
    }

    return &KphpObjectTypes[index];
}

/**
 * \brief Acquires the atomic object reference lock shared.
 *
 * \param[in,out] ObjectRef The object reference to acquire the lock for.
 */
_Requires_lock_not_held_(*ObjectRef)
_Acquires_lock_(*ObjectRef)
FORCEINLINE
VOID KphpAtomicAcquireObjectLockShared(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    for (;; YieldProcessor())
    {
        ULONG_PTR object;
        ULONG_PTR lock;

        object = ReadULongPtrAcquire(&ObjectRef->Object);

        lock = object & KPH_ATOMIC_OBJECT_REF_LOCK_MASK;

        if (lock >= KPH_ATOMIC_OBJECT_REF_SHARED_MAX)
        {
            continue;
        }

        if (InterlockedCompareExchangeULongPtr(&ObjectRef->Object,
                                               object + 1,
                                               object) == object)
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
_Requires_lock_held_(*ObjectRef)
_Releases_lock_(*ObjectRef)
FORCEINLINE
VOID KphpAtomicReleaseObjectLockShared(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    ULONG_PTR object;

    object = InterlockedDecrementULongPtr(&ObjectRef->Object);

    object = object & KPH_ATOMIC_OBJECT_REF_SHARED_MAX;

    NT_ASSERT(object < KPH_ATOMIC_OBJECT_REF_SHARED_MAX);
}

/**
 * \brief Acquires the atomic object reference lock exclusive.
 *
 * \param[in,out] ObjectRef The object reference to acquire the lock for.
 */
_Requires_lock_not_held_(*ObjectRef)
_Acquires_lock_(*ObjectRef)
FORCEINLINE
VOID KphpAtomicAcquireObjectLockExclusive(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    ULONG_PTR object;

    for (;; YieldProcessor())
    {
        ULONG_PTR locked;

        object = ReadULongPtrAcquire(&ObjectRef->Object);

        if (object & KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_FLAG)
        {
            continue;
        }

        locked = object | KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_FLAG;

        if (InterlockedCompareExchangeULongPtr(&ObjectRef->Object,
                                               locked,
                                               object) == object)
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
_Requires_lock_held_(*ObjectRef)
_Releases_lock_(*ObjectRef)
FORCEINLINE
VOID KphpAtomicReleaseObjectLockExclusive(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    BOOLEAN result;

    result = InterlockedBitTestAndResetULongPtr(&ObjectRef->Object,
                                                KPH_ATOMIC_OBJECT_REF_EXCLUSIVE_BIT);

    NT_ASSERT(result);
}

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
_Must_inspect_result_
PVOID KphAtomicReferenceObject(
    _In_ PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    PVOID object;

    KphpAtomicAcquireObjectLockShared(ObjectRef);

    object = (PVOID)(ObjectRef->Object & KPH_ATOMIC_OBJECT_REF_OBJECT_MASK);
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
_Must_inspect_result_
PVOID KphpAtomicStoreObjectReference(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef,
    _In_opt_ PVOID Object
    )
{
    PVOID previous;
    ULONG_PTR object;

    NT_ASSERT(((ULONG_PTR)Object & KPH_ATOMIC_OBJECT_REF_LOCK_MASK) == 0);

    KphpAtomicAcquireObjectLockExclusive(ObjectRef);

    previous = (PVOID)(ObjectRef->Object & KPH_ATOMIC_OBJECT_REF_OBJECT_MASK);

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
VOID KphAtomicAssignObjectReference(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef,
    _In_opt_ PVOID Object
    )
{
    PVOID previous;

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
_Must_inspect_result_
PVOID KphAtomicMoveObjectReference(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef,
    _In_opt_ PVOID Object
    )
{
    return KphpAtomicStoreObjectReference(ObjectRef, Object);
}

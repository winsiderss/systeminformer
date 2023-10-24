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

#include <trace.h>

//
// N.B. If more object types are added the array must be expanded.
//

static volatile LONG KphpObjectTypeCount = 0;
static KPH_OBJECT_TYPE KphpObjectTypes[9];

#define KPH_ATOMIC_OBJECT_LOCKED_MASK (((ULONG_PTR)-1) - 1)

C_ASSERT(ARRAYSIZE(KphpObjectTypes) < MAXUCHAR);

/**
 * \brief Creates an object type.
 *
 * \param[in] TypeName The name of the type, this must always be resident.
 * Preferably a string literal.
 * \param[in] TypeInfo The information for the type being created.
 * \param[out] ObjectType Set to a pointer to the object type on success.
 */
VOID
KphCreateObjectType(
    _In_ PUNICODE_STRING TypeName,
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
NTSTATUS
KphCreateObject(
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
VOID
KphReferenceObject(
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
VOID
KphDereferenceObject(
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
PKPH_OBJECT_TYPE
KphGetObjectType(
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
 * \brief Acquires the atomic object reference lock.
 *
 * \param[in,out] ObjectRef The object reference to acquire the lock for.
 */
_Acquires_lock_(_Global_critical_region_)
VOID KphpAtomicAcquireObjectLock(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_)
    PKPH_ATOMIC_OBJECT_REF ObjectRef
    )
{
    KeEnterCriticalRegion();

    for (;; YieldProcessor())
    {
        ULONG_PTR object;
        PVOID expected;
        PVOID locked;

        object = ObjectRef->Object;
        MemoryBarrier();

        if (object & ~KPH_ATOMIC_OBJECT_LOCKED_MASK)
        {
            continue;
        }

        expected = (PVOID)object;
        locked = (PVOID)((ULONG_PTR)expected | ~KPH_ATOMIC_OBJECT_LOCKED_MASK);

        if (InterlockedCompareExchangePointer((PVOID*)&ObjectRef->Object,
                                              locked,
                                              expected) == expected)
        {
            break;
        }
    }
}

/**
 * \brief Releases the atomic object reference lock.
 *
 * \param[in,out] ObjectRef The object reference to release the lock of.
 */
_Releases_lock_(_Global_critical_region_)
VOID KphpAtomicReleaseObjectLock(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_)
    PKPH_ATOMIC_OBJECT_REF Entry
    )
{
    ULONG_PTR object;
    PVOID unlocked;

    object = Entry->Object;
    MemoryBarrier();

    NT_ASSERT(object & ~KPH_ATOMIC_OBJECT_LOCKED_MASK);

    unlocked = (PVOID)(object & KPH_ATOMIC_OBJECT_LOCKED_MASK);

    InterlockedExchangePointer((PVOID*)&Entry->Object, unlocked);

    KeLeaveCriticalRegion();
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

    KphpAtomicAcquireObjectLock(ObjectRef);

    object = (PVOID)(ObjectRef->Object & KPH_ATOMIC_OBJECT_LOCKED_MASK);
    if (object)
    {
        KphReferenceObject(object);
    }

    KphpAtomicReleaseObjectLock(ObjectRef);

    return object;
}

/**
 * \brief Assigns an object to an atomically managed object reference.
 *
 * \details This mechanism provides a light weight and fast way to atomically
 * managed a reference to an object. Any previously managed reference will be
 * released. This function will acquire an additional reference to the object
 * the caller should still release their reference.
 *
 * \param[in,out] ObjectRef The object reference to assign the object to.
 * \param[in] Object The object to reference and assign.
 */
VOID KphAtomicAssignObjectReference(
    _Inout_ PKPH_ATOMIC_OBJECT_REF ObjectRef,
    _In_ PVOID Object
    )
{
    ULONG_PTR object;
    PVOID previous;

    KphReferenceObject(Object);

    KphpAtomicAcquireObjectLock(ObjectRef);

    previous = (PVOID)(ObjectRef->Object & KPH_ATOMIC_OBJECT_LOCKED_MASK);

    object = ((ULONG_PTR)Object | ~KPH_ATOMIC_OBJECT_LOCKED_MASK);

    InterlockedExchangePointer((PVOID*)&ObjectRef->Object, (PVOID)object);

    KphpAtomicReleaseObjectLock(ObjectRef);

    if (previous)
    {
        KphDereferenceObject(previous);
    }
}

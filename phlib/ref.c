/*
 * Process Hacker -
 *   object manager
 *
 * Copyright (C) 2009-2011 wj32
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _PH_REF_PRIVATE
#include <phbase.h>
#include <phintrnl.h>
#include <refp.h>

/** The type object type. */
PPH_OBJECT_TYPE PhObjectTypeObject = NULL;

/** The next object to delete. */
PPH_OBJECT_HEADER PhObjectNextToFree = NULL;

/** Free list for small objects. */
PH_FREE_LIST PhObjectSmallFreeList;

/** The allocated memory object type. */
PPH_OBJECT_TYPE PhAllocType = NULL;

static ULONG PhpAutoPoolTlsIndex;

#ifdef DEBUG
LIST_ENTRY PhDbgObjectListHead;
PH_QUEUED_LOCK PhDbgObjectListLock = PH_QUEUED_LOCK_INIT;
PPH_CREATE_OBJECT_HOOK PhDbgCreateObjectHook = NULL;
#endif

#define REF_STAT_UP(Name) PHLIB_INC_STATISTIC(Name)

/**
 * Initializes the object manager module.
 */
NTSTATUS PhInitializeRef(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PH_OBJECT_TYPE dummyObjectType;

#ifdef DEBUG
    InitializeListHead(&PhDbgObjectListHead);
#endif

    PhInitializeFreeList(
        &PhObjectSmallFreeList,
        PhpAddObjectHeaderSize(PHOBJ_SMALL_OBJECT_SIZE),
        PHOBJ_SMALL_OBJECT_COUNT
        );

    // Create the fundamental object type.

    memset(&dummyObjectType, 0, sizeof(PH_OBJECT_TYPE));
    PhObjectTypeObject = &dummyObjectType; // PhCreateObject expects an object type.

    status = PhCreateObjectType(
        &PhObjectTypeObject,
        L"Type",
        0,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Now that the fundamental object type exists, fix it up.
    PhObjectToObjectHeader(PhObjectTypeObject)->Type = PhObjectTypeObject;
    PhObjectTypeObject->NumberOfObjects = 1;

    // Create the allocated memory object type.
    status = PhCreateObjectType(
        &PhAllocType,
        L"Alloc",
        0,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Reserve a slot for the auto pool.
    PhpAutoPoolTlsIndex = TlsAlloc();

    if (PhpAutoPoolTlsIndex == TLS_OUT_OF_INDEXES)
        return STATUS_INSUFFICIENT_RESOURCES;

    return status;
}

/**
 * Allocates a object.
 *
 * \param Object A variable which receives a pointer to the newly allocated object.
 * \param ObjectSize The size of the object.
 * \param Flags A combination of flags specifying how the object is to be allocated.
 * \li \c PHOBJ_RAISE_ON_FAIL An exception will be raised if the object cannot be
 * allocated.
 * \param ObjectType The type of the object.
 */
__mayRaise NTSTATUS PhCreateObject(
    __out PVOID *Object,
    __in SIZE_T ObjectSize,
    __in ULONG Flags,
    __in PPH_OBJECT_TYPE ObjectType
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PPH_OBJECT_HEADER objectHeader;

#ifdef PHOBJ_STRICT_CHECKS
    /* Check the flags. */
    if ((Flags & PHOBJ_VALID_FLAGS) != Flags) /* Valid flag mask */
    {
        status = STATUS_INVALID_PARAMETER_3;
    }
#else
    assert(!((Flags & PHOBJ_VALID_FLAGS) != Flags));
#endif

#ifdef PHOBJ_STRICT_CHECKS
    if (NT_SUCCESS(status))
    {
#endif
        /* Allocate storage for the object. Note that this includes
         * the object header followed by the object body. */
        objectHeader = PhpAllocateObject(ObjectType, ObjectSize, Flags);

#ifndef PHOBJ_ALLOCATE_NEVER_NULL
        if (!objectHeader)
            status = STATUS_NO_MEMORY;
#endif
#ifdef PHOBJ_STRICT_CHECKS
    }
#endif

#ifndef PHOBJ_ALLOCATE_NEVER_NULL
    if (!NT_SUCCESS(status))
    {
        if (!(Flags & PHOBJ_RAISE_ON_FAIL))
            return status;
        else
            PhRaiseStatus(status);
    }
#endif

    /* Object type statistics. */
    _InterlockedIncrement((PLONG)&ObjectType->NumberOfObjects);

    /* Initialize the object header. */
    objectHeader->RefCount = 1;
    // objectHeader->Flags is initialized by PhpAllocateObject.
    objectHeader->Size = ObjectSize;
    objectHeader->Type = ObjectType;

    REF_STAT_UP(RefObjectsCreated);

#ifdef DEBUG
    {
        USHORT capturedFrames;

        capturedFrames = RtlCaptureStackBackTrace(1, 16, objectHeader->StackBackTrace, NULL);
        memset(
            &objectHeader->StackBackTrace[capturedFrames],
            0,
            sizeof(objectHeader->StackBackTrace) - capturedFrames * sizeof(PVOID)
            );
    }

    PhAcquireQueuedLockExclusive(&PhDbgObjectListLock);
    InsertTailList(&PhDbgObjectListHead, &objectHeader->ObjectListEntry);
    PhReleaseQueuedLockExclusive(&PhDbgObjectListLock);

    {
        PPH_CREATE_OBJECT_HOOK dbgCreateObjectHook;

        dbgCreateObjectHook = PhDbgCreateObjectHook;

        if (dbgCreateObjectHook)
        {
            dbgCreateObjectHook(
                PhObjectHeaderToObject(objectHeader),
                ObjectSize,
                Flags,
                ObjectType
                );
        }
    }
#endif

    /* Pass a pointer to the object body back to the caller. */
    *Object = PhObjectHeaderToObject(objectHeader);

    return status;
}

/**
 * References the specified object.
 *
 * \param Object A pointer to the object to reference.
 */
VOID PhReferenceObject(
    __in PVOID Object
    )
{
    PPH_OBJECT_HEADER objectHeader;

    objectHeader = PhObjectToObjectHeader(Object);
    /* Increment the reference count. */
    _InterlockedIncrement(&objectHeader->RefCount);
}

/**
 * References the specified object.
 *
 * \param Object A pointer to the object to reference.
 * \param RefCount The number of references to add.
 *
 * \return The new reference count of the object.
 */
__mayRaise LONG PhReferenceObjectEx(
    __in PVOID Object,
    __in LONG RefCount
    )
{
    PPH_OBJECT_HEADER objectHeader;
    LONG oldRefCount;

#ifdef PHOBJ_STRICT_CHECKS
    /* Make sure we're not adding a negative reference count. */
    if (RefCount < 0)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);
#else
    assert(!(RefCount < 0));
#endif

    objectHeader = PhObjectToObjectHeader(Object);
    /* Increase the reference count. */
    oldRefCount = _InterlockedExchangeAdd(&objectHeader->RefCount, RefCount);

    return oldRefCount + RefCount;
}

/**
 * Attempts to reference an object and fails if it is being
 * destroyed.
 *
 * \param Object The object to reference if it is not being deleted.
 *
 * \return TRUE if the object was referenced, FALSE if
 * it was being deleted and was not referenced.
 *
 * \remarks
 * This function is useful if a reference to an object is
 * held, protected by a mutex, and the delete procedure of
 * the object's type attempts to acquire the mutex. If this
 * function is called while the mutex is owned, you can
 * avoid referencing an object that is being destroyed.
 */
BOOLEAN PhReferenceObjectSafe(
    __in PVOID Object
    )
{
    PPH_OBJECT_HEADER objectHeader;
    BOOLEAN result;

    objectHeader = PhObjectToObjectHeader(Object);
    /* Increase the reference count only if it isn't 0 (atomically). */
    result = PhpInterlockedIncrementSafe(&objectHeader->RefCount);

    return result;
}

/**
 * Dereferences the specified object.
 * The object will be freed if its reference count reaches 0.
 *
 * \param Object A pointer to the object to dereference.
 *
 * \return TRUE if the object was freed, otherwise FALSE.
 */
VOID PhDereferenceObject(
    __in PVOID Object
    )
{
    PPH_OBJECT_HEADER objectHeader;
    LONG newRefCount;

    objectHeader = PhObjectToObjectHeader(Object);
    /* Decrement the reference count. */
    newRefCount = _InterlockedDecrement(&objectHeader->RefCount);
    ASSUME_ASSERT(newRefCount >= 0);

    /* Free the object if it has 0 references. */
    if (newRefCount == 0)
    {
        PhpFreeObject(objectHeader);
    }
}

/**
 * Dereferences the specified object.
 * The object will be freed in a worker thread if its reference count
 * reaches 0.
 *
 * \param Object A pointer to the object to dereference.
 *
 * \return TRUE if the object was freed, otherwise FALSE.
 */
BOOLEAN PhDereferenceObjectDeferDelete(
    __in PVOID Object
    )
{
    return PhDereferenceObjectEx(Object, 1, TRUE) == 0;
}

/**
 * Dereferences the specified object.
 * The object will be freed if its reference count reaches 0.
 *
 * \param Object A pointer to the object to dereference.
 * \param RefCount The number of references to remove.
 * \param DeferDelete Whether to defer deletion of the object.
 *
 * \return The new reference count of the object.
 */
__mayRaise LONG PhDereferenceObjectEx(
    __in PVOID Object,
    __in LONG RefCount,
    __in BOOLEAN DeferDelete
    )
{
    PPH_OBJECT_HEADER objectHeader;
    LONG oldRefCount;
    LONG newRefCount;

#ifdef PHOBJ_STRICT_CHECKS
    /* Make sure we're not subtracting a negative reference count. */
    if (RefCount < 0)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);
#else
    assert(!(RefCount < 0));
#endif

    objectHeader = PhObjectToObjectHeader(Object);

    /* Decrease the reference count. */
    oldRefCount = _InterlockedExchangeAdd(&objectHeader->RefCount, -RefCount);
    newRefCount = oldRefCount - RefCount;

    /* Free the object if it has 0 references. */
    if (newRefCount == 0)
    {
        if (DeferDelete)
        {
            PhpDeferDeleteObject(objectHeader);
        }
        else
        {
            /* Free the object. */
            PhpFreeObject(objectHeader);
        }
    }
    else if (newRefCount < 0)
    {
        PhRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    return newRefCount;
}

/**
 * Gets an object's type.
 *
 * \param Object A pointer to an object.
 *
 * \return A pointer to a type object.
 */
PPH_OBJECT_TYPE PhGetObjectType(
    __in PVOID Object
    )
{
    return PhObjectToObjectHeader(Object)->Type;
}

/**
 * Creates an object type.
 *
 * \param ObjectType A variable which receives a pointer to the newly
 * created object type.
 * \param Name The name of the type.
 * \param Flags A combination of flags affecting the behaviour of the
 * object type.
 * \param DeleteProcedure A callback function that is executed when
 * an object of this type is about to be freed (i.e. when its
 * reference count is 0).
 *
 * \remarks Do not reference or dereference the object type once it
 * is created.
 */
NTSTATUS PhCreateObjectType(
    __out PPH_OBJECT_TYPE *ObjectType,
    __in PWSTR Name,
    __in ULONG Flags,
    __in_opt PPH_TYPE_DELETE_PROCEDURE DeleteProcedure
    )
{
    return PhCreateObjectTypeEx(
        ObjectType,
        Name,
        Flags,
        DeleteProcedure,
        NULL
        );
}

/**
 * Creates an object type.
 *
 * \param ObjectType A variable which receives a pointer to the newly
 * created object type.
 * \param Name The name of the type.
 * \param Flags A combination of flags affecting the behaviour of the
 * object type.
 * \param DeleteProcedure A callback function that is executed when
 * an object of this type is about to be freed (i.e. when its
 * reference count is 0).
 * \param Parameters A structure containing additional parameters
 * for the object type.
 *
 * \remarks Do not reference or dereference the object type once it
 * is created.
 */
NTSTATUS PhCreateObjectTypeEx(
    __out PPH_OBJECT_TYPE *ObjectType,
    __in PWSTR Name,
    __in ULONG Flags,
    __in_opt PPH_TYPE_DELETE_PROCEDURE DeleteProcedure,
    __in_opt PPH_OBJECT_TYPE_PARAMETERS Parameters
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PPH_OBJECT_TYPE objectType;

    /* Check the flags. */
    if ((Flags & PHOBJTYPE_VALID_FLAGS) != Flags) /* Valid flag mask */
        return STATUS_INVALID_PARAMETER_3;
    if ((Flags & PHOBJTYPE_USE_FREE_LIST) && !Parameters)
        return STATUS_INVALID_PARAMETER_MIX;

    /* Create the type object. */
    status = PhCreateObject(
        &objectType,
        sizeof(PH_OBJECT_TYPE),
        0,
        PhObjectTypeObject
        );

    if (!NT_SUCCESS(status))
        return status;

    /* Initialize the type object. */
    objectType->Flags = Flags;
    objectType->DeleteProcedure = DeleteProcedure;
    objectType->Name = Name;
    objectType->NumberOfObjects = 0;

    if (Parameters)
    {
        if (Flags & PHOBJTYPE_USE_FREE_LIST)
        {
            PhInitializeFreeList(
                &objectType->FreeList,
                PhpAddObjectHeaderSize(Parameters->FreeListSize),
                Parameters->FreeListCount
                );
        }
    }

    *ObjectType = objectType;

    return status;
}

/**
 * Gets information about an object type.
 *
 * \param ObjectType A pointer to an object type.
 * \param Information A variable which receives
 * information about the object type.
 */
VOID PhGetObjectTypeInformation(
    __in PPH_OBJECT_TYPE ObjectType,
    __out PPH_OBJECT_TYPE_INFORMATION Information
    )
{
    Information->Name = ObjectType->Name;
    Information->NumberOfObjects = ObjectType->NumberOfObjects;
}

/**
 * Allocates storage for an object.
 *
 * \param ObjectType The type of the object.
 * \param ObjectSize The size of the object, excluding the header.
 * \param Flags A combination of flags specifying how the object is to be allocated.
 */
PPH_OBJECT_HEADER PhpAllocateObject(
    __in PPH_OBJECT_TYPE ObjectType,
    __in SIZE_T ObjectSize,
    __in ULONG Flags
    )
{
    PPH_OBJECT_HEADER objectHeader;

    if (ObjectType->Flags & PHOBJTYPE_USE_FREE_LIST)
    {
#ifdef PHOBJ_STRICT_CHECKS
        if (ObjectType->FreeList.Size != PhpAddObjectHeaderSize(ObjectSize))
            PhRaiseStatus(STATUS_INVALID_PARAMETER);
#else
        assert(ObjectType->FreeList.Size == PhpAddObjectHeaderSize(ObjectSize));
#endif

        objectHeader = PhAllocateFromFreeList(&ObjectType->FreeList);
        objectHeader->Flags = PHOBJ_FROM_TYPE_FREE_LIST;
        REF_STAT_UP(RefObjectsAllocatedFromTypeFreeList);
    }
    else if (ObjectSize <= PHOBJ_SMALL_OBJECT_SIZE)
    {
        objectHeader = PhAllocateFromFreeList(&PhObjectSmallFreeList);
        objectHeader->Flags = PHOBJ_FROM_SMALL_FREE_LIST;
        REF_STAT_UP(RefObjectsAllocatedFromSmallFreeList);
    }
    else
    {
        objectHeader = PhAllocate(PhpAddObjectHeaderSize(ObjectSize));
        objectHeader->Flags = 0;
        REF_STAT_UP(RefObjectsAllocated);
    }

    return objectHeader;
}

/**
 * Calls the delete procedure for an object and frees its
 * allocated storage.
 *
 * \param ObjectHeader A pointer to the object header of an allocated object.
 */
VOID PhpFreeObject(
    __in PPH_OBJECT_HEADER ObjectHeader
    )
{
    /* Object type statistics. */
    _InterlockedDecrement(&ObjectHeader->Type->NumberOfObjects);

#ifdef DEBUG
    PhAcquireQueuedLockExclusive(&PhDbgObjectListLock);
    RemoveEntryList(&ObjectHeader->ObjectListEntry);
    PhReleaseQueuedLockExclusive(&PhDbgObjectListLock);
#endif

    REF_STAT_UP(RefObjectsDestroyed);

    /* Call the delete procedure if we have one. */
    if (ObjectHeader->Type->DeleteProcedure)
    {
        ObjectHeader->Type->DeleteProcedure(
            PhObjectHeaderToObject(ObjectHeader),
            0
            );
    }

    if (ObjectHeader->Flags & PHOBJ_FROM_TYPE_FREE_LIST)
    {
        PhFreeToFreeList(&ObjectHeader->Type->FreeList, ObjectHeader);
        REF_STAT_UP(RefObjectsFreedToTypeFreeList);
    }
    else if (ObjectHeader->Flags & PHOBJ_FROM_SMALL_FREE_LIST)
    {
        PhFreeToFreeList(&PhObjectSmallFreeList, ObjectHeader);
        REF_STAT_UP(RefObjectsFreedToSmallFreeList);
    }
    else
    {
        PhFree(ObjectHeader);
        REF_STAT_UP(RefObjectsFreed);
    }
}

/**
 * Queues an object for deletion.
 *
 * \param ObjectHeader A pointer to the object header of the object
 * to delete.
 */
VOID PhpDeferDeleteObject(
    __in PPH_OBJECT_HEADER ObjectHeader
    )
{
    PPH_OBJECT_HEADER nextToFree;

    /* Add the object to the list while saving the old value, atomically.
     * Note that it is first-in, last-out.
     */
    while (TRUE)
    {
        nextToFree = PhObjectNextToFree;
        ObjectHeader->NextToFree = nextToFree;

        /* Attempt to set the global next-to-free variable. */
        if (_InterlockedCompareExchangePointer(
            &PhObjectNextToFree,
            ObjectHeader,
            nextToFree
            ) == nextToFree)
        {
            /* Success. */
            break;
        }

        /* Someone else changed the next-to-free variable.
         * Go back and try again.
         */
    }

    REF_STAT_UP(RefObjectsDeleteDeferred);

    /* Was the to-free list empty before? If so, we need to queue
     * a work item.
     */
    if (!nextToFree)
    {
        PhQueueItemGlobalWorkQueue(PhpDeferDeleteObjectRoutine, NULL);
    }
}

/**
 * Removes and frees objects from the to-free list.
 */
NTSTATUS PhpDeferDeleteObjectRoutine(
    __in PVOID Parameter
    )
{
    PPH_OBJECT_HEADER objectHeader;
    PPH_OBJECT_HEADER nextObjectHeader;

    // Clear the list and obtain the first object to free.
    objectHeader = _InterlockedExchangePointer(&PhObjectNextToFree, NULL);

    while (objectHeader)
    {
        nextObjectHeader = objectHeader->NextToFree;
        PhpFreeObject(objectHeader);
        objectHeader = nextObjectHeader;
    }

    return STATUS_SUCCESS;
}

/**
 * Creates a reference-counted memory block.
 *
 * \param Alloc A variable which receives a pointer to the
 * memory block.
 * \param Size The number of bytes to allocate.
 */
NTSTATUS PhCreateAlloc(
    __out PVOID *Alloc,
    __in SIZE_T Size
    )
{
    return PhCreateObject(
        Alloc,
        Size,
        0,
        PhAllocType
        );
}

/**
 * Gets the current auto-dereference pool for the
 * current thread.
 */
FORCEINLINE PPH_AUTO_POOL PhpGetCurrentAutoPool(
    VOID
    )
{
    return (PPH_AUTO_POOL)TlsGetValue(PhpAutoPoolTlsIndex);
}

/**
 * Sets the current auto-dereference pool for the
 * current thread.
 */
__mayRaise FORCEINLINE VOID PhpSetCurrentAutoPool(
    __in PPH_AUTO_POOL AutoPool
    )
{
    if (!TlsSetValue(PhpAutoPoolTlsIndex, AutoPool))
        PhRaiseStatus(STATUS_UNSUCCESSFUL);

#ifdef DEBUG
    {
        PPHP_BASE_THREAD_DBG dbg;

        dbg = (PPHP_BASE_THREAD_DBG)TlsGetValue(PhDbgThreadDbgTlsIndex);

        if (dbg)
        {
            dbg->CurrentAutoPool = AutoPool;
        }
    }
#endif
}

/**
 * Initializes an auto-dereference pool and sets it
 * as the current pool for the current thread. You
 * must call PhDeleteAutoPool() before storage for
 * the auto-dereference pool is freed.
 *
 * \remarks Always store auto-dereference pools in local
 * variables, and do not share the pool with any other
 * functions.
 */
VOID PhInitializeAutoPool(
    __out PPH_AUTO_POOL AutoPool
    )
{
    AutoPool->StaticCount = 0;
    AutoPool->DynamicCount = 0;
    AutoPool->DynamicAllocated = 0;
    AutoPool->DynamicObjects = NULL;

    // Add the pool to the stack.
    AutoPool->NextPool = PhpGetCurrentAutoPool();
    PhpSetCurrentAutoPool(AutoPool);

    REF_STAT_UP(RefAutoPoolsCreated);
}

/**
 * Deletes an auto-dereference pool.
 * The function will dereference any objects
 * currently in the pool. If a pool other than
 * the current pool is passed to the function,
 * an exception is raised.
 *
 * \param AutoPool The auto-dereference pool to delete.
 */
__mayRaise VOID PhDeleteAutoPool(
    __inout PPH_AUTO_POOL AutoPool
    )
{
    PhDrainAutoPool(AutoPool);

    if (PhpGetCurrentAutoPool() != AutoPool)
        PhRaiseStatus(STATUS_UNSUCCESSFUL);

    // Remove the pool from the stack.
    PhpSetCurrentAutoPool(AutoPool->NextPool);

    // Free the dynamic array if it hasn't been freed yet.
    if (AutoPool->DynamicObjects)
        PhFree(AutoPool->DynamicObjects);

    REF_STAT_UP(RefAutoPoolsDestroyed);
}

/**
 * Adds an object to the current auto-dereference
 * pool for the current thread.
 * If the current thread does not have an auto-dereference
 * pool, the function raises an exception.
 *
 * \param Object A pointer to an object. The object
 * will be dereferenced when the current auto-dereference
 * pool is drained or freed.
 */
__mayRaise VOID PhaDereferenceObject(
    __in PVOID Object
    )
{
    PPH_AUTO_POOL autoPool = PhpGetCurrentAutoPool();

#ifdef DEBUG
    // If we don't have an auto-dereference pool,
    // we don't want to leak the object (unlike what
    // Apple does with NSAutoreleasePool).
    if (!autoPool)
        PhRaiseStatus(STATUS_UNSUCCESSFUL);
#endif

    // See if we can use the static array.
    if (autoPool->StaticCount < PH_AUTO_POOL_STATIC_SIZE)
    {
        autoPool->StaticObjects[autoPool->StaticCount++] = Object;
        return;
    }

    // Use the dynamic array.

    // Allocate the array if we haven't already.
    if (!autoPool->DynamicObjects)
    {
        autoPool->DynamicAllocated = 64;
        autoPool->DynamicObjects = PhAllocate(
            sizeof(PVOID) * autoPool->DynamicAllocated
            );
        REF_STAT_UP(RefAutoPoolsDynamicAllocated);
    }

    // See if we need to resize the array.
    if (autoPool->DynamicCount == autoPool->DynamicAllocated)
    {
        autoPool->DynamicAllocated *= 2;
        autoPool->DynamicObjects = PhReAllocate(
            autoPool->DynamicObjects,
            sizeof(PVOID) * autoPool->DynamicAllocated
            );
        REF_STAT_UP(RefAutoPoolsDynamicResized);
    }

    autoPool->DynamicObjects[autoPool->DynamicCount++] = Object;
}

/**
 * Dereferences and removes all objects in an
 * auto-release pool.
 *
 * \param AutoPool The auto-release pool to drain.
 */
VOID PhDrainAutoPool(
    __in PPH_AUTO_POOL AutoPool
    )
{
    ULONG i;

    for (i = 0; i < AutoPool->StaticCount; i++)
        PhDereferenceObject(AutoPool->StaticObjects[i]);

    AutoPool->StaticCount = 0;

    if (AutoPool->DynamicObjects)
    {
        for (i = 0; i < AutoPool->DynamicCount; i++)
        {
            PhDereferenceObject(AutoPool->DynamicObjects[i]);
        }

        AutoPool->DynamicCount = 0;

        if (AutoPool->DynamicAllocated > PH_AUTO_POOL_DYNAMIC_BIG_SIZE)
        {
            AutoPool->DynamicAllocated = 0;
            PhFree(AutoPool->DynamicObjects);
            AutoPool->DynamicObjects = NULL;
        }
    }
}

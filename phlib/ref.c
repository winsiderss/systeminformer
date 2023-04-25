/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *
 */

#include <phbase.h>
#include <refp.h>

#include <phintrnl.h>
#include <workqueue.h>

PPH_OBJECT_TYPE PhObjectTypeObject = NULL;
SLIST_HEADER PhObjectDeferDeleteListHead;
PH_FREE_LIST PhObjectSmallFreeList;
PPH_OBJECT_TYPE PhAllocType = NULL;

ULONG PhObjectTypeCount = 0;
PPH_OBJECT_TYPE PhObjectTypeTable[PH_OBJECT_TYPE_TABLE_SIZE];

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
BOOLEAN PhRefInitialization(
    VOID
    )
{
    PH_OBJECT_TYPE dummyObjectType;

#ifdef DEBUG
    InitializeListHead(&PhDbgObjectListHead);
#endif

    RtlInitializeSListHead(&PhObjectDeferDeleteListHead);
    PhInitializeFreeList(
        &PhObjectSmallFreeList,
        PhAddObjectHeaderSize(PH_OBJECT_SMALL_OBJECT_SIZE),
        PH_OBJECT_SMALL_OBJECT_COUNT
        );

    // Create the fundamental object type.

    memset(&dummyObjectType, 0, sizeof(PH_OBJECT_TYPE));
    PhObjectTypeObject = &dummyObjectType; // PhCreateObject expects an object type.
    PhObjectTypeTable[0] = &dummyObjectType; // PhCreateObject also expects PhObjectTypeTable[0] to be filled in.
    PhObjectTypeObject = PhCreateObjectType(L"Type", 0, NULL);

    // Now that the fundamental object type exists, fix it up.
    PhObjectToObjectHeader(PhObjectTypeObject)->TypeIndex = PhObjectTypeObject->TypeIndex;
    PhObjectTypeObject->NumberOfObjects = 1;

    // Create the allocated memory object type.
    PhAllocType = PhCreateObjectType(L"Alloc", 0, NULL);

    // Reserve a slot for the auto pool.
    PhpAutoPoolTlsIndex = TlsAlloc();

    if (PhpAutoPoolTlsIndex == TLS_OUT_OF_INDEXES)
        return FALSE;

    return TRUE;
}

/**
 * Allocates a object.
 *
 * \param ObjectSize The size of the object.
 * \param ObjectType The type of the object.
 *
 * \return A pointer to the newly allocated object.
 */
_May_raise_ PVOID PhCreateObject(
    _In_ SIZE_T ObjectSize,
    _In_ PPH_OBJECT_TYPE ObjectType
    )
{
    PPH_OBJECT_HEADER objectHeader;

    // Allocate storage for the object. Note that this includes the object header followed by the
    // object body.
    objectHeader = PhpAllocateObject(ObjectType, ObjectSize);

    // Object type statistics.
    _InterlockedIncrement((PLONG)&ObjectType->NumberOfObjects);

    // Initialize the object header.
    objectHeader->RefCount = 1;
    objectHeader->TypeIndex = ObjectType->TypeIndex;
    // objectHeader->Flags is set by PhpAllocateObject.

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
                0,
                ObjectType
                );
        }
    }
#endif

    return PhObjectHeaderToObject(objectHeader);
}

/**
 * References the specified object.
 *
 * \param Object A pointer to the object to reference.
 *
 * \return The object.
 */
PVOID PhReferenceObject(
    _In_ PVOID Object
    )
{
    PPH_OBJECT_HEADER objectHeader;

    objectHeader = PhObjectToObjectHeader(Object);
    // Increment the reference count.
    _InterlockedIncrement(&objectHeader->RefCount);

    return Object;
}

/**
 * References the specified object.
 *
 * \param Object A pointer to the object to reference.
 * \param RefCount The number of references to add.
 *
 * \return The object.
 */
_May_raise_ PVOID PhReferenceObjectEx(
    _In_ PVOID Object,
    _In_ LONG RefCount
    )
{
    PPH_OBJECT_HEADER objectHeader;
    LONG oldRefCount;

    assert(!(RefCount < 0));

    objectHeader = PhObjectToObjectHeader(Object);
    // Increase the reference count.
    oldRefCount = _InterlockedExchangeAdd(&objectHeader->RefCount, RefCount);

    return Object;
}

/**
 * Attempts to reference an object and fails if it is being destroyed.
 *
 * \param Object The object to reference if it is not being deleted.
 *
 * \return The object itself if the object was referenced, NULL if it was being deleted and was not
 * referenced.
 *
 * \remarks This function is useful if a reference to an object is held, protected by a mutex, and
 * the delete procedure of the object's type attempts to acquire the mutex. If this function is
 * called while the mutex is owned, you can avoid referencing an object that is being destroyed.
 */
PVOID PhReferenceObjectSafe(
    _In_ PVOID Object
    )
{
    PPH_OBJECT_HEADER objectHeader;

    objectHeader = PhObjectToObjectHeader(Object);

    // Increase the reference count only if it positive already (atomically).
    if (PhpInterlockedIncrementSafe(&objectHeader->RefCount))
        return Object;
    else
        return NULL;
}

/**
 * Dereferences the specified object.
 * The object will be freed if its reference count reaches 0.
 *
 * \param Object A pointer to the object to dereference.
 */
VOID PhDereferenceObject(
    _In_ PVOID Object
    )
{
    PPH_OBJECT_HEADER objectHeader;
    LONG newRefCount;

    objectHeader = PhObjectToObjectHeader(Object);
    // Decrement the reference count.
    newRefCount = _InterlockedDecrement(&objectHeader->RefCount);
    ASSUME_ASSERT(newRefCount >= 0);
    ASSUME_ASSERT(!(newRefCount < 0));

    // Free the object if it has 0 references.
    if (newRefCount == 0)
    {
        PhpFreeObject(objectHeader);
    }
}

/**
 * Dereferences the specified object.
 * The object will be freed in a worker thread if its reference count reaches 0.
 *
 * \param Object A pointer to the object to dereference.
 */
VOID PhDereferenceObjectDeferDelete(
    _In_ PVOID Object
    )
{
    PhDereferenceObjectEx(Object, 1, TRUE);
}

/**
 * Dereferences the specified object.
 * The object will be freed if its reference count reaches 0.
 *
 * \param Object A pointer to the object to dereference.
 * \param RefCount The number of references to remove.
 * \param DeferDelete Whether to defer deletion of the object.
 */
_May_raise_ VOID PhDereferenceObjectEx(
    _In_ PVOID Object,
    _In_ LONG RefCount,
    _In_ BOOLEAN DeferDelete
    )
{
    PPH_OBJECT_HEADER objectHeader;
    LONG oldRefCount;
    LONG newRefCount;

    assert(!(RefCount < 0));

    objectHeader = PhObjectToObjectHeader(Object);

    // Decrease the reference count.
    oldRefCount = _InterlockedExchangeAdd(&objectHeader->RefCount, -RefCount);
    newRefCount = oldRefCount - RefCount;

    // Free the object if it has 0 references.
    if (newRefCount == 0)
    {
        if (DeferDelete)
        {
            PhpDeferDeleteObject(objectHeader);
        }
        else
        {
            // Free the object.
            PhpFreeObject(objectHeader);
        }
    }
    else if (newRefCount < 0)
    {
        PhRaiseStatus(STATUS_INVALID_PARAMETER);
    }
}

/**
 * Gets an object's type.
 *
 * \param Object A pointer to an object.
 *
 * \return A pointer to a type object.
 */
PPH_OBJECT_TYPE PhGetObjectType(
    _In_ PVOID Object
    )
{
    return PhObjectTypeTable[PhObjectToObjectHeader(Object)->TypeIndex];
}

/**
 * Creates an object type.
 *
 * \param Name The name of the type.
 * \param Flags A combination of flags affecting the behaviour of the object type.
 * \param DeleteProcedure A callback function that is executed when an object of this type is about
 * to be freed (i.e. when its reference count is 0).
 *
 * \return A pointer to the newly created object type.
 *
 * \remarks Do not reference or dereference the object type once it is created.
 */
PPH_OBJECT_TYPE PhCreateObjectType(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ PPH_TYPE_DELETE_PROCEDURE DeleteProcedure
    )
{
    return PhCreateObjectTypeEx(
        Name,
        Flags,
        DeleteProcedure,
        NULL
        );
}

/**
 * Creates an object type.
 *
 * \param Name The name of the type.
 * \param Flags A combination of flags affecting the behaviour of the object type.
 * \param DeleteProcedure A callback function that is executed when an object of this type is about
 * to be freed (i.e. when its reference count is 0).
 * \param Parameters A structure containing additional parameters for the object type.
 *
 * \return A pointer to the newly created object type.
 *
 * \remarks Do not reference or dereference the object type once it is created.
 */
PPH_OBJECT_TYPE PhCreateObjectTypeEx(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ PPH_TYPE_DELETE_PROCEDURE DeleteProcedure,
    _In_opt_ PPH_OBJECT_TYPE_PARAMETERS Parameters
    )
{
    PPH_OBJECT_TYPE objectType;

    // Check the flags.
    if ((Flags & PH_OBJECT_TYPE_VALID_FLAGS) != Flags) /* Valid flag mask */
        PhRaiseStatus(STATUS_INVALID_PARAMETER_3);
    if ((Flags & PH_OBJECT_TYPE_USE_FREE_LIST) && !Parameters)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_MIX);

    // Create the type object.
    objectType = PhCreateObject(sizeof(PH_OBJECT_TYPE), PhObjectTypeObject);

    // Initialize the type object.
    objectType->Flags = (USHORT)Flags;
    objectType->TypeIndex = (USHORT)_InterlockedIncrement(&PhObjectTypeCount) - 1;
    objectType->NumberOfObjects = 0;
    objectType->DeleteProcedure = DeleteProcedure;
    objectType->Name = Name;

    PhObjectTypeTable[objectType->TypeIndex] = objectType;

    if (Parameters)
    {
        if (Flags & PH_OBJECT_TYPE_USE_FREE_LIST)
        {
            PhInitializeFreeList(
                &objectType->FreeList,
                PhAddObjectHeaderSize(Parameters->FreeListSize),
                Parameters->FreeListCount
                );
        }
    }

    return objectType;
}

/**
 * Gets information about an object type.
 *
 * \param ObjectType A pointer to an object type.
 * \param Information A variable which receives information about the object type.
 */
VOID PhGetObjectTypeInformation(
    _In_ PPH_OBJECT_TYPE ObjectType,
    _Out_ PPH_OBJECT_TYPE_INFORMATION Information
    )
{
    Information->Name = ObjectType->Name;
    Information->NumberOfObjects = ObjectType->NumberOfObjects;
    Information->Flags = ObjectType->Flags;
    Information->TypeIndex = ObjectType->TypeIndex;
}

/**
 * Allocates storage for an object.
 *
 * \param ObjectType The type of the object.
 * \param ObjectSize The size of the object, excluding the header.
 */
PPH_OBJECT_HEADER PhpAllocateObject(
    _In_ PPH_OBJECT_TYPE ObjectType,
    _In_ SIZE_T ObjectSize
    )
{
    PPH_OBJECT_HEADER objectHeader;

    if (ObjectType->Flags & PH_OBJECT_TYPE_USE_FREE_LIST)
    {
        assert(ObjectType->FreeList.Size == PhAddObjectHeaderSize(ObjectSize));

        objectHeader = PhAllocateFromFreeList(&ObjectType->FreeList);
        objectHeader->Flags = PH_OBJECT_FROM_TYPE_FREE_LIST;
        REF_STAT_UP(RefObjectsAllocatedFromTypeFreeList);
    }
    else if (ObjectSize <= PH_OBJECT_SMALL_OBJECT_SIZE)
    {
        objectHeader = PhAllocateFromFreeList(&PhObjectSmallFreeList);
        objectHeader->Flags = PH_OBJECT_FROM_SMALL_FREE_LIST;
        REF_STAT_UP(RefObjectsAllocatedFromSmallFreeList);
    }
    else
    {
        objectHeader = PhAllocate(PhAddObjectHeaderSize(ObjectSize));
        objectHeader->Flags = 0;
        REF_STAT_UP(RefObjectsAllocated);
    }

    return objectHeader;
}

/**
 * Calls the delete procedure for an object and frees its allocated storage.
 *
 * \param ObjectHeader A pointer to the object header of an allocated object.
 */
VOID PhpFreeObject(
    _In_ PPH_OBJECT_HEADER ObjectHeader
    )
{
    PPH_OBJECT_TYPE objectType;

    objectType = PhObjectTypeTable[ObjectHeader->TypeIndex];

    // Object type statistics.
    _InterlockedDecrement(&objectType->NumberOfObjects);

#ifdef DEBUG
    PhAcquireQueuedLockExclusive(&PhDbgObjectListLock);
    RemoveEntryList(&ObjectHeader->ObjectListEntry);
    PhReleaseQueuedLockExclusive(&PhDbgObjectListLock);
#endif

    REF_STAT_UP(RefObjectsDestroyed);

    // Call the delete procedure if we have one.
    if (objectType->DeleteProcedure)
    {
        objectType->DeleteProcedure(PhObjectHeaderToObject(ObjectHeader), 0);
    }

    if (ObjectHeader->Flags & PH_OBJECT_FROM_TYPE_FREE_LIST)
    {
        PhFreeToFreeList(&objectType->FreeList, ObjectHeader);
        REF_STAT_UP(RefObjectsFreedToTypeFreeList);
    }
    else if (ObjectHeader->Flags & PH_OBJECT_FROM_SMALL_FREE_LIST)
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
 * \param ObjectHeader A pointer to the object header of the object to delete.
 */
VOID PhpDeferDeleteObject(
    _In_ PPH_OBJECT_HEADER ObjectHeader
    )
{
    PSLIST_ENTRY oldFirstEntry;

    // Save TypeIndex and Flags since they get overwritten when we push the object onto the defer
    // delete list.
    ObjectHeader->DeferDelete = 1;
    MemoryBarrier();
    ObjectHeader->SavedTypeIndex = ObjectHeader->TypeIndex;
    ObjectHeader->SavedFlags = ObjectHeader->Flags;

    oldFirstEntry = RtlFirstEntrySList(&PhObjectDeferDeleteListHead);
    RtlInterlockedPushEntrySList(&PhObjectDeferDeleteListHead, &ObjectHeader->DeferDeleteListEntry);
    REF_STAT_UP(RefObjectsDeleteDeferred);

    // Was the to-free list empty before? If so, we need to queue a work item.
    if (!oldFirstEntry)
    {
        PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), PhpDeferDeleteObjectRoutine, NULL);
    }
}

/**
 * Removes and frees objects from the to-free list.
 */
NTSTATUS PhpDeferDeleteObjectRoutine(
    _In_ PVOID Parameter
    )
{
    PSLIST_ENTRY listEntry;
    PPH_OBJECT_HEADER objectHeader;

    // Clear the list and obtain the first object to free.
    listEntry = RtlInterlockedFlushSList(&PhObjectDeferDeleteListHead);

    while (listEntry)
    {
        objectHeader = CONTAINING_RECORD(listEntry, PH_OBJECT_HEADER, DeferDeleteListEntry);
        listEntry = listEntry->Next;

        // Restore TypeIndex and Flags.
        objectHeader->TypeIndex = (USHORT)objectHeader->SavedTypeIndex;
        objectHeader->Flags = (UCHAR)objectHeader->SavedFlags;

        PhpFreeObject(objectHeader);
    }

    return STATUS_SUCCESS;
}

/**
 * Creates a reference-counted memory block.
 *
 * \param Size The number of bytes to allocate.
 *
 * \return A pointer to the memory block.
 */
PVOID PhCreateAlloc(
    _In_ SIZE_T Size
    )
{
    return PhCreateObject(Size, PhAllocType);
}

/**
 * Gets the current auto-dereference pool for the current thread.
 */
FORCEINLINE PPH_AUTO_POOL PhpGetCurrentAutoPool(
    VOID
    )
{
    return (PPH_AUTO_POOL)TlsGetValue(PhpAutoPoolTlsIndex);
}

/**
 * Sets the current auto-dereference pool for the current thread.
 */
_May_raise_ FORCEINLINE VOID PhpSetCurrentAutoPool(
    _In_ PPH_AUTO_POOL AutoPool
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
 * Initializes an auto-dereference pool and sets it as the current pool for the current thread. You
 * must call PhDeleteAutoPool() before storage for the auto-dereference pool is freed.
 *
 * \remarks Always store auto-dereference pools in local variables, and do not share the pool with
 * any other functions.
 */
VOID PhInitializeAutoPool(
    _Out_ PPH_AUTO_POOL AutoPool
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
 * Deletes an auto-dereference pool. The function will dereference any objects currently in the
 * pool. If a pool other than the current pool is passed to the function, an exception is raised.
 *
 * \param AutoPool The auto-dereference pool to delete.
 */
_May_raise_ VOID PhDeleteAutoPool(
    _Inout_ PPH_AUTO_POOL AutoPool
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
 * Dereferences and removes all objects in an auto-release pool.
 *
 * \param AutoPool The auto-release pool to drain.
 */
VOID PhDrainAutoPool(
    _In_ PPH_AUTO_POOL AutoPool
    )
{
    PhDereferenceObjects(AutoPool->StaticObjects, AutoPool->StaticCount);
    AutoPool->StaticCount = 0;

    if (AutoPool->DynamicObjects)
    {
        PhDereferenceObjects(AutoPool->DynamicObjects, AutoPool->DynamicCount);
        AutoPool->DynamicCount = 0;

        if (AutoPool->DynamicAllocated > PH_AUTO_POOL_DYNAMIC_BIG_SIZE)
        {
            AutoPool->DynamicAllocated = 0;
            PhFree(AutoPool->DynamicObjects);
            AutoPool->DynamicObjects = NULL;
        }
    }
}

/**
 * Adds an object to the current auto-dereference pool for the current thread. If the current thread
 * does not have an auto-dereference pool, the function raises an exception.
 *
 * \param Object A pointer to an object. The object will be dereferenced when the current
 * auto-dereference pool is drained or freed.
 */
_May_raise_ PVOID PhAutoDereferenceObject(
    _In_opt_ PVOID Object
    )
{
    PPH_AUTO_POOL autoPool = PhpGetCurrentAutoPool();

#ifdef DEBUG
    // If we don't have an auto-dereference pool, we don't want to leak the object (unlike what
    // Apple does with NSAutoreleasePool).
    if (!autoPool)
        PhRaiseStatus(STATUS_UNSUCCESSFUL);
#endif

    if (!Object)
        return NULL;

    // See if we can use the static array.
    if (autoPool->StaticCount < PH_AUTO_POOL_STATIC_SIZE)
    {
        autoPool->StaticObjects[autoPool->StaticCount++] = Object;
        return Object;
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

    return Object;
}

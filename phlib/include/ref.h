/*
 * Process Hacker -
 *   internal object manager
 *
 * Copyright (C) 2009-2015 wj32
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

#ifndef _PH_REF_H
#define _PH_REF_H

#ifdef __cplusplus
extern "C" {
#endif

// Configuration

#define PH_OBJECT_SMALL_OBJECT_SIZE 48
#define PH_OBJECT_SMALL_OBJECT_COUNT 512

// Object type flags
#define PH_OBJECT_TYPE_USE_FREE_LIST 0x00000001
#define PH_OBJECT_TYPE_VALID_FLAGS 0x00000001

// Object type callbacks

/**
 * The delete procedure for an object type, called when
 * an object of the type is being freed.
 *
 * \param Object A pointer to the object being freed.
 * \param Flags Reserved.
 */
typedef VOID (NTAPI *PPH_TYPE_DELETE_PROCEDURE)(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

struct _PH_OBJECT_TYPE;
typedef struct _PH_OBJECT_TYPE *PPH_OBJECT_TYPE;

struct _PH_QUEUED_LOCK;
typedef struct _PH_QUEUED_LOCK PH_QUEUED_LOCK, *PPH_QUEUED_LOCK;

#ifdef DEBUG
typedef VOID (NTAPI *PPH_CREATE_OBJECT_HOOK)(
    _In_ PVOID Object,
    _In_ SIZE_T Size,
    _In_ ULONG Flags,
    _In_ PPH_OBJECT_TYPE ObjectType
    );
#endif

#ifndef _PH_REF_PRIVATE
extern PPH_OBJECT_TYPE PhObjectTypeObject;
extern PPH_OBJECT_TYPE PhAllocType;

#ifdef DEBUG
extern LIST_ENTRY PhDbgObjectListHead;
extern PH_QUEUED_LOCK PhDbgObjectListLock;
extern PPH_CREATE_OBJECT_HOOK PhDbgCreateObjectHook;
#endif
#endif

typedef struct _PH_OBJECT_TYPE_PARAMETERS
{
    SIZE_T FreeListSize;
    ULONG FreeListCount;
} PH_OBJECT_TYPE_PARAMETERS, *PPH_OBJECT_TYPE_PARAMETERS;

typedef struct _PH_OBJECT_TYPE_INFORMATION
{
    PWSTR Name;
    ULONG NumberOfObjects;
    USHORT Flags;
    UCHAR TypeIndex;
    UCHAR Reserved;
} PH_OBJECT_TYPE_INFORMATION, *PPH_OBJECT_TYPE_INFORMATION;

NTSTATUS PhInitializeRef(
    VOID
    );

_May_raise_
PHLIBAPI
PVOID
NTAPI
PhCreateObject(
    _In_ SIZE_T ObjectSize,
    _In_ PPH_OBJECT_TYPE ObjectType
    );

PHLIBAPI
PVOID
NTAPI
PhReferenceObject(
    _In_ PVOID Object
    );

_May_raise_
PHLIBAPI
LONG
NTAPI
PhReferenceObjectEx(
    _In_ PVOID Object,
    _In_ LONG RefCount
    );

PHLIBAPI
BOOLEAN
NTAPI
PhReferenceObjectSafe(
    _In_ PVOID Object
    );

PHLIBAPI
VOID
NTAPI
PhDereferenceObject(
    _In_ PVOID Object
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDereferenceObjectDeferDelete(
    _In_ PVOID Object
    );

_May_raise_
PHLIBAPI
LONG
NTAPI
PhDereferenceObjectEx(
    _In_ PVOID Object,
    _In_ LONG RefCount,
    _In_ BOOLEAN DeferDelete
    );

PHLIBAPI
PPH_OBJECT_TYPE
NTAPI
PhGetObjectType(
    _In_ PVOID Object
    );

PHLIBAPI
PPH_OBJECT_TYPE
NTAPI
PhCreateObjectType(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ PPH_TYPE_DELETE_PROCEDURE DeleteProcedure
    );

PHLIBAPI
PPH_OBJECT_TYPE
NTAPI
PhCreateObjectTypeEx(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ PPH_TYPE_DELETE_PROCEDURE DeleteProcedure,
    _In_opt_ PPH_OBJECT_TYPE_PARAMETERS Parameters
    );

PHLIBAPI
VOID
NTAPI
PhGetObjectTypeInformation(
    _In_ PPH_OBJECT_TYPE ObjectType,
    _Out_ PPH_OBJECT_TYPE_INFORMATION Information
    );

PHLIBAPI
PVOID
NTAPI
PhCreateAlloc(
    _In_ SIZE_T Size
    );

// Object reference functions

FORCEINLINE
VOID
PhSwapReference(
    _Inout_ PVOID *ObjectReference,
    _In_opt_ PVOID NewObject
    )
{
    PVOID oldObject;

    oldObject = *ObjectReference;
    *ObjectReference = NewObject;

    if (NewObject) PhReferenceObject(NewObject);
    if (oldObject) PhDereferenceObject(oldObject);
}

FORCEINLINE
VOID
PhMoveReference(
    _Inout_ PVOID *ObjectReference,
    _In_opt_ _Assume_refs_(1) PVOID NewObject
    )
{
    PVOID oldObject;

    oldObject = *ObjectReference;
    *ObjectReference = NewObject;

    if (oldObject) PhDereferenceObject(oldObject);
}

FORCEINLINE
VOID
PhSetReference(
    _Out_ PVOID *ObjectReference,
    _In_opt_ PVOID NewObject
    )
{
    *ObjectReference = NewObject;

    if (NewObject) PhReferenceObject(NewObject);
}

FORCEINLINE
VOID
PhClearReference(
    _Inout_ PVOID *ObjectReference
    )
{
    PhMoveReference(ObjectReference, NULL);
}

// Auto-dereference pool

/** The size of the static array in an auto-release pool. */
#define PH_AUTO_POOL_STATIC_SIZE 64
/** The maximum size of the dynamic array for it to be
 * kept after the auto-release pool is drained. */
#define PH_AUTO_POOL_DYNAMIC_BIG_SIZE 256

/**
 * An auto-dereference pool can be used for
 * semi-automatic reference counting. Batches of
 * objects are dereferenced at a certain time.
 *
 * This object is not thread-safe and cannot
 * be used across thread boundaries. Always
 * store them as local variables.
 */
typedef struct _PH_AUTO_POOL
{
    ULONG StaticCount;
    PVOID StaticObjects[PH_AUTO_POOL_STATIC_SIZE];

    ULONG DynamicCount;
    ULONG DynamicAllocated;
    PVOID *DynamicObjects;

    struct _PH_AUTO_POOL *NextPool;
} PH_AUTO_POOL, *PPH_AUTO_POOL;

PHLIBAPI
VOID
NTAPI
PhInitializeAutoPool(
    _Out_ PPH_AUTO_POOL AutoPool
    );

_May_raise_
PHLIBAPI
VOID
NTAPI
PhDeleteAutoPool(
    _Inout_ PPH_AUTO_POOL AutoPool
    );

PHLIBAPI
VOID
NTAPI
PhDrainAutoPool(
    _In_ PPH_AUTO_POOL AutoPool
    );

_May_raise_
PHLIBAPI
PVOID
NTAPI
PhAutoDereferenceObject(
    _In_opt_ PVOID Object
    );

/** Deprecated. Use PhAutoDereferenceObject instead. */
PHLIBAPI VOID NTAPI PhaDereferenceObject(PVOID Object);

#ifdef __cplusplus
}
#endif

#endif

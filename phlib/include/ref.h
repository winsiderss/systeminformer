/*
 * Process Hacker - 
 *   internal object manager
 * 
 * Copyright (C) 2009 wj32
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

#define PHOBJ_SMALL_OBJECT_SIZE 48
#define PHOBJ_SMALL_OBJECT_COUNT 512

//#define PHOBJ_STRICT_CHECKS
#define PHOBJ_ALLOCATE_NEVER_NULL

/* Object flags */
#define PHOBJ_RAISE_ON_FAIL 0x00000001
#define PHOBJ_VALID_FLAGS 0x00000001

/* Object type flags */
#define PHOBJTYPE_USE_FREE_LIST 0x00000001
#define PHOBJTYPE_SECURED 0x00000002
#define PHOBJTYPE_VALID_FLAGS 0x00000003

/* Object type callbacks */

/**
 * The delete procedure for an object type, called when 
 * an object of the type is being freed.
 * 
 * \param Object A pointer to the object being freed.
 * \param Flags Reserved.
 */
typedef VOID (NTAPI *PPH_TYPE_DELETE_PROCEDURE)(
    __in PVOID Object,
    __in ULONG Flags
    );

struct _PH_OBJECT_TYPE;
typedef struct _PH_OBJECT_TYPE *PPH_OBJECT_TYPE;

struct _PH_QUEUED_LOCK;
typedef struct _PH_QUEUED_LOCK PH_QUEUED_LOCK, *PPH_QUEUED_LOCK;

#ifdef DEBUG
typedef VOID (NTAPI *PPH_CREATE_OBJECT_HOOK)(
    __in PVOID Object,
    __in SIZE_T Size,
    __in ULONG Flags,
    __in PPH_OBJECT_TYPE ObjectType
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

    UCHAR OffsetOfSecurityDescriptor;
    GENERIC_MAPPING GenericMapping;
} PH_OBJECT_TYPE_PARAMETERS, *PPH_OBJECT_TYPE_PARAMETERS;

typedef struct _PH_OBJECT_TYPE_INFORMATION
{
    PWSTR Name;
    ULONG NumberOfObjects;
} PH_OBJECT_TYPE_INFORMATION, *PPH_OBJECT_TYPE_INFORMATION;

NTSTATUS PhInitializeRef();

__mayRaise
PHLIBAPI
NTSTATUS
NTAPI
PhCreateObject(
    __out PVOID *Object,
    __in SIZE_T ObjectSize,
    __in ULONG Flags,
    __in PPH_OBJECT_TYPE ObjectType
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateObjectType(
    __out PPH_OBJECT_TYPE *ObjectType,
    __in PWSTR Name,
    __in ULONG Flags,
    __in_opt PPH_TYPE_DELETE_PROCEDURE DeleteProcedure
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateObjectTypeEx(
    __out PPH_OBJECT_TYPE *ObjectType,
    __in PWSTR Name,
    __in ULONG Flags,
    __in_opt PPH_TYPE_DELETE_PROCEDURE DeleteProcedure,
    __in_opt PPH_OBJECT_TYPE_PARAMETERS Parameters
    );

PHLIBAPI
VOID
NTAPI
PhDereferenceObject(
    __in PVOID Object
    );

PHLIBAPI
BOOLEAN
NTAPI
PhDereferenceObjectDeferDelete(
    __in PVOID Object
    );

__mayRaise
PHLIBAPI
LONG
NTAPI
PhDereferenceObjectEx(
    __in PVOID Object,
    __in LONG RefCount,
    __in BOOLEAN DeferDelete
    );

PHLIBAPI
PPH_OBJECT_TYPE
NTAPI
PhGetObjectType(
    __in PVOID Object
    );

PHLIBAPI
VOID
NTAPI
PhGetObjectTypeInformation(
    __in PPH_OBJECT_TYPE ObjectType,
    __out PPH_OBJECT_TYPE_INFORMATION Information
    );

PHLIBAPI
VOID
NTAPI
PhReferenceObject(
    __in PVOID Object
    );

__mayRaise
PHLIBAPI
LONG
NTAPI
PhReferenceObjectEx(
    __in PVOID Object,
    __in LONG RefCount
    );

PHLIBAPI
BOOLEAN
NTAPI
PhReferenceObjectSafe(
    __in PVOID Object
    );

PHLIBAPI
NTSTATUS
NTAPI
PhQuerySecurityObject(
    __in PVOID Object,
    __in SECURITY_INFORMATION SecurityInformation,
    __out_opt PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in ULONG BufferLength,
    __out PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSetSecurityObject(
    __in PVOID Object,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in_opt HANDLE TokenHandle
    );

FORCEINLINE VOID PhSwapReference(
    __inout PPVOID ObjectReference,
    __in PVOID NewObject
    )
{
    PVOID oldObject;

    oldObject = *ObjectReference;
    *ObjectReference = NewObject;

    if (NewObject) PhReferenceObject(NewObject);
    if (oldObject) PhDereferenceObject(oldObject);
}

FORCEINLINE VOID PhSwapReference2(
    __inout PPVOID ObjectReference,
    __in __assumeRefs(1) PVOID NewObject
    )
{
    PVOID oldObject;

    oldObject = *ObjectReference;
    *ObjectReference = NewObject;

    if (oldObject) PhDereferenceObject(oldObject);
}

PHLIBAPI
NTSTATUS
NTAPI
PhCreateAlloc(
    __out PVOID *Alloc,
    __in SIZE_T Size
    );

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
    PPVOID DynamicObjects;

    struct _PH_AUTO_POOL *NextPool;
} PH_AUTO_POOL, *PPH_AUTO_POOL;

PHLIBAPI
VOID
NTAPI
PhInitializeAutoPool(
    __out PPH_AUTO_POOL AutoPool
    );

__mayRaise
PHLIBAPI
VOID
NTAPI
PhDeleteAutoPool(
    __inout PPH_AUTO_POOL AutoPool
    );

__mayRaise
PHLIBAPI
VOID
NTAPI
PhaDereferenceObject(
    __in PVOID Object
    );

PHLIBAPI
VOID
NTAPI
PhDrainAutoPool(
    __in PPH_AUTO_POOL AutoPool
    );

/**
 * Calls PhaDereferenceObject() and returns the given object.
 *
 * \param Object A pointer to an object. The value can be 
 * null; in that case no action is performed.
 *
 * \return The value of \a Object.
 */
FORCEINLINE PVOID PHA_DEREFERENCE(
    __in PVOID Object
    )
{
    if (Object)
        PhaDereferenceObject(Object);

    return Object;
}

#ifdef __cplusplus
}
#endif

#endif

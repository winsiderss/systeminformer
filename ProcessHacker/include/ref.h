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

#ifndef REF_H
#define REF_H

#include <phbase.h>

/* Object flags */
#define PHOBJ_RAISE_ON_FAIL 0x00000001
#define PHOBJ_VALID_FLAGS 0x00000001

/* Object type flags */
#define PHOBJTYPE_VALID_FLAGS 0x00000000

/* Object type callbacks */

/**
 * The delete procedure for an object type, called when 
 * an object of the type is being freed.
 * 
 * \param Object A pointer to the object being freed.
 * \param Flags The flags specified when the object was created.
 */
typedef VOID (NTAPI *PPH_TYPE_DELETE_PROCEDURE)(
    __in PVOID Object,
    __in ULONG Flags
    );

struct _PH_OBJECT_TYPE;
typedef struct _PH_OBJECT_TYPE *PPH_OBJECT_TYPE;

struct _PH_AUTO_POOL;
typedef struct _PH_AUTO_POOL *PPH_AUTO_POOL;

struct _PH_FAST_LOCK;
typedef struct _PH_FAST_LOCK PH_FAST_LOCK, *PPH_FAST_LOCK;

#ifdef DEBUG
typedef VOID (NTAPI *PPH_CREATE_OBJECT_HOOK)(
    __in PVOID Object,
    __in SIZE_T Size,
    __in ULONG Flags,
    __in PPH_OBJECT_TYPE ObjectType
    );
#endif

#ifndef REF_PRIVATE
extern PPH_OBJECT_TYPE PhObjectTypeObject;
extern PPH_OBJECT_TYPE PhAllocType;

#ifdef DEBUG
extern LIST_ENTRY PhObjectListHead;
extern PH_FAST_LOCK PhObjectListLock;
extern PPH_CREATE_OBJECT_HOOK PhCreateObjectHook;
#endif
#endif

typedef struct _PH_OBJECT_TYPE_INFORMATION
{
    ULONG NumberOfObjects;
} PH_OBJECT_TYPE_INFORMATION, *PPH_OBJECT_TYPE_INFORMATION;

NTSTATUS PhInitializeRef();

NTSTATUS PhCreateObject(
    __out PVOID *Object,
    __in SIZE_T ObjectSize,
    __in ULONG Flags,
    __in_opt PPH_OBJECT_TYPE ObjectType,
    __in_opt LONG AdditionalReferences
    );

NTSTATUS PhCreateObjectType(
    __out PPH_OBJECT_TYPE *ObjectType,
    __in PWSTR Name,
    __in ULONG Flags,
    __in_opt PPH_TYPE_DELETE_PROCEDURE DeleteProcedure
    );

VOID PhDereferenceObject(
    __in PVOID Object
    );

BOOLEAN PhDereferenceObjectDeferDelete(
    __in PVOID Object
    );

LONG PhDereferenceObjectEx(
    __in PVOID Object,
    __in LONG RefCount,
    __in BOOLEAN DeferDelete
    );

PPH_OBJECT_TYPE PhGetObjectType(
    __in PVOID Object
    );

VOID PhGetObjectTypeInformation(
    __in PPH_OBJECT_TYPE ObjectType,
    __out PPH_OBJECT_TYPE_INFORMATION Information
    );

VOID PhReferenceObject(
    __in PVOID Object
    );

LONG PhReferenceObjectEx(
    __in PVOID Object,
    __in LONG RefCount
    );

BOOLEAN PhReferenceObjectSafe(
    __in PVOID Object
    );

FORCEINLINE VOID PhSwapReference(
    __inout PPVOID ObjectReference,
    __in PVOID NewObject
    )
{
    PVOID oldObject;

    oldObject = *ObjectReference;
    *ObjectReference = NewObject;

    if (oldObject) PhDereferenceObject(oldObject);
    if (NewObject) PhReferenceObject(NewObject);
}

FORCEINLINE VOID PhSwapReference2(
    __inout PPVOID ObjectReference,
    __in PVOID NewObject
    )
{
    PVOID oldObject;

    oldObject = *ObjectReference;
    *ObjectReference = NewObject;

    if (oldObject) PhDereferenceObject(oldObject);
}

NTSTATUS PhCreateAlloc(
    __out PVOID *Alloc,
    __in SIZE_T Size
    );

PPH_AUTO_POOL PhCreateAutoPool();

VOID PhFreeAutoPool(
    __inout PPH_AUTO_POOL AutoPool
    );

VOID PhaDereferenceObject(
    __in PVOID Object
    );

VOID PhDrainAutoPool(
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

#endif

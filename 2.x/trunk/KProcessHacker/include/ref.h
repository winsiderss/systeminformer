/*
 * Process Hacker Driver - 
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

#ifndef _REF_H
#define _REF_H

#include "kph.h"

/* Object flags */
#define KPHOBJ_RAISE_ON_FAIL 0x00000001
#define KPHOBJ_PAGED_POOL 0x00000002
#define KPHOBJ_NONPAGED_POOL 0x00000004
#define KPHOBJ_VALID_FLAGS 0x00000007

/* Object type flags */
#define KPHOBJTYPE_PASSIVE_LEVEL_DELETE 0x00000001
#define KPHOBJTYPE_VALID_FLAGS 0x00000001

/* Object type callbacks */

/* PKPH_TYPE_DELETE_PROCEDURE
 * 
 * The delete procedure for an object type, called when 
 * an object of the type is being freed.
 * 
 * Object: A pointer to the object being freed.
 * Flags: The flags specified when the object was created.
 * 
 * IRQL: = PASSIVE_LEVEL if the require passive level flag was 
 * specified for the object type, otherwise <= APC_LEVEL.
 */
typedef VOID (NTAPI *PKPH_TYPE_DELETE_PROCEDURE)(
    __in PVOID Object,
    __in ULONG Flags
    );

struct _KPH_OBJECT_TYPE;
typedef struct _KPH_OBJECT_TYPE *PKPH_OBJECT_TYPE;

#ifndef _REF_PRIVATE
extern PKPH_OBJECT_TYPE KphObjectTypeObject;
#endif

NTSTATUS KphRefInit();

NTSTATUS KphRefDeinit();

NTSTATUS KphCreateObject(
    __out PVOID *Object,
    __in SIZE_T ObjectSize,
    __in ULONG Flags,
    __in_opt PKPH_OBJECT_TYPE ObjectType,
    __in_opt LONG AdditionalReferences
    );

NTSTATUS KphCreateObjectType(
    __out PKPH_OBJECT_TYPE *ObjectType,
    __in POOL_TYPE DefaultPoolType,
    __in ULONG Flags,
    __in PKPH_TYPE_DELETE_PROCEDURE DeleteProcedure
    );

BOOLEAN KphDereferenceObject(
    __in PVOID Object
    );

BOOLEAN KphDereferenceObjectDeferDelete(
    __in PVOID Object
    );

LONG KphDereferenceObjectEx(
    __in PVOID Object,
    __in LONG RefCount,
    __in BOOLEAN DeferDelete
    );

PKPH_OBJECT_TYPE KphGetObjectType(
    __in PVOID Object
    );

VOID KphReferenceObject(
    __in PVOID Object
    );

LONG KphReferenceObjectEx(
    __in PVOID Object,
    __in LONG RefCount
    );

BOOLEAN KphReferenceObjectSafe(
    __in PVOID Object
    );

#endif

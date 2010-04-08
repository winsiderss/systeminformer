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

#ifndef _REFP_H
#define _REFP_H

#define TAG_KPHOBJ ('bOhP')

#define KphObjectToObjectHeader(Object) ((PKPH_OBJECT_HEADER)CONTAINING_RECORD((PCHAR)(Object), KPH_OBJECT_HEADER, Body))
#define KphObjectHeaderToObject(ObjectHeader) (&((PKPH_OBJECT_HEADER)(ObjectHeader))->Body)
#define KphpAddObjectHeaderSize(Size) ((Size) + FIELD_OFFSET(KPH_OBJECT_HEADER, Body))

typedef struct _KPH_OBJECT_HEADER *PKPH_OBJECT_HEADER;
typedef struct _KPH_OBJECT_TYPE *PKPH_OBJECT_TYPE;

typedef struct _KPH_OBJECT_HEADER
{
    /* The reference count of the object. */
    LONG RefCount;
    /* The flags that were used to create the object. */
    ULONG Flags;
    union
    {
        /* The size of the object, excluding the header. */
        SIZE_T Size;
        /* A pointer to the object header of the next object to free. */
        PKPH_OBJECT_HEADER NextToFree;
    };
    /* The type of the object. */
    PKPH_OBJECT_TYPE Type;
    /* A linked list entry for an optional object manager object list. 
     * For example, this may be used to free all objects when the 
     * driver exits.
     */
    LIST_ENTRY GlobalObjectListEntry;
    
    /* The body of the object. For use by the KphObject(Header)ToObject(Header) macros. */
    QUAD Body;
} KPH_OBJECT_HEADER, *PKPH_OBJECT_HEADER;

typedef struct _KPH_OBJECT_TYPE
{
    /* The default pool type for objects of this type, used when the 
     * pool type is not specified when an object is created. */
    POOL_TYPE DefaultPoolType;
    /* The flags that were used to create the object type. */
    ULONG Flags;
    /* An optional procedure called when objects of this type are freed. */
    PKPH_TYPE_DELETE_PROCEDURE DeleteProcedure;
    
    /* The total number of objects of this type that are alive. */
    ULONG NumberOfObjects;
} KPH_OBJECT_TYPE, *PKPH_OBJECT_TYPE;

/* KphpInterlockedIncrementSafe
 * 
 * Increments a reference count, but will never increment 
 * from 0 to 1.
 */
FORCEINLINE BOOLEAN KphpInterlockedIncrementSafe(
    __inout PLONG RefCount
    )
{
    LONG refCount;
    
    /* Here we will attempt to increment the reference count, 
     * making sure that it is not 0.
     */
    
    while (TRUE)
    {
        refCount = *RefCount;
        
        /* Check if the reference count is 0. If it is, the 
         * object is being or about to be deleted.
         */
        if (refCount == 0)
            return FALSE;
        
        /* Try to increment the reference count. */
        if (InterlockedCompareExchange(
            RefCount,
            refCount + 1,
            refCount
            ) == refCount)
        {
            /* Success. */
            return TRUE;
        }
        
        /* Someone else changed the reference count before we did. 
         * Go back and try again.
         */
    }
}

PKPH_OBJECT_HEADER KphpAllocateObject(
    __in SIZE_T ObjectSize,
    __in POOL_TYPE PoolType
    );

VOID KphpDeferDeleteObject(
    __in PKPH_OBJECT_HEADER ObjectHeader
    );

VOID KphpDeferDeleteObjectRoutine(
    __in PVOID Parameter
    );

VOID KphpFreeObject(
    __in PKPH_OBJECT_HEADER ObjectHeader
    );

#endif

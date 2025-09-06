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

#ifndef _PH_REFP_H
#define _PH_REFP_H

#define PH_OBJECT_TYPE_TABLE_SIZE 256

/** The object was allocated from the small free list. */
#define PH_OBJECT_FROM_SMALL_FREE_LIST 0x1
/** The object was allocated from the type free list. */
#define PH_OBJECT_FROM_TYPE_FREE_LIST 0x2

/**
 * The object header contains object manager information including the reference count of an object
 * and its type.
 */
typedef struct _PH_OBJECT_HEADER
{
    union
    {
        struct
        {
            USHORT TypeIndex;
            UCHAR Flags;
            UCHAR Reserved1;
#ifdef _WIN64
            ULONG Reserved2;
#endif
            union
            {
                LONG RefCount;
                struct
                {
                    LONG SavedTypeIndex : 16;
                    LONG SavedFlags : 8;
                    LONG Reserved : 7;
                    LONG DeferDelete : 1; // MUST be the high bit, so that RefCount < 0 when deferring delete
                };
            };
#ifdef _WIN64
            ULONG Reserved3;
#endif
        };
        SLIST_ENTRY DeferDeleteListEntry;
    };

#ifdef DEBUG
    PVOID StackBackTrace[16];
    LIST_ENTRY ObjectListEntry;
#endif

    /**
     * The body of the object. For use by the \ref PhObjectToObjectHeader and
     * \ref PhObjectHeaderToObject macros.
     */
    QUAD_PTR Body;
} PH_OBJECT_HEADER, *PPH_OBJECT_HEADER;

static_assert((FIELD_OFFSET(PH_OBJECT_HEADER, Body) % MEMORY_ALLOCATION_ALIGNMENT) == 0, "PH_OBJECT_HEADER alignment invalid");

#ifdef _WIN64
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, TypeIndex) == 0x0, "PH_OBJECT_HEADER.TypeIndex offset mismatch");
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, Flags) == 0x2, "PH_OBJECT_HEADER.Flags offset mismatch");
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, Reserved1) == 0x3, "PH_OBJECT_HEADER.Reserved1 offset mismatch");
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, Reserved2) == 0x4, "PH_OBJECT_HEADER.Reserved2 offset mismatch");
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, RefCount) == 0x8, "PH_OBJECT_HEADER.RefCount offset mismatch");
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, Reserved3) == 0xc, "PH_OBJECT_HEADER.Reserved3 offset mismatch");
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, DeferDeleteListEntry) == 0x0, "PH_OBJECT_HEADER.DeferDeleteListEntry offset mismatch");
#if defined(DEBUG)
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, Body) == 0xA0, "PH_OBJECT_HEADER.Body offset mismatch");
#else
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, Body) == 0x10, "PH_OBJECT_HEADER.Body offset mismatch");
#endif
#else
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, TypeIndex) == 0x0, "PH_OBJECT_HEADER.TypeIndex offset mismatch");
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, Flags) == 0x2, "PH_OBJECT_HEADER.Flags offset mismatch");
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, Reserved1) == 0x3, "PH_OBJECT_HEADER.Reserved1 offset mismatch");
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, RefCount) == 0x4, "PH_OBJECT_HEADER.RefCount offset mismatch");
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, DeferDeleteListEntry) == 0x0, "PH_OBJECT_HEADER.DeferDeleteListEntry offset mismatch");
#if defined(DEBUG)
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, Body) == 0x50, "PH_OBJECT_HEADER.Body offset mismatch");
#else
static_assert(FIELD_OFFSET(PH_OBJECT_HEADER, Body) == 0x8, "PH_OBJECT_HEADER.Body offset mismatch");
#endif
#endif

/**
 * Gets a pointer to the object header for an object.
 *
 * \param Object A pointer to an object.
 *
 * \return A pointer to the object header of the object.
 */
#if defined(_PHLIB_)
#define PhObjectToObjectHeader(Object) ((PPH_OBJECT_HEADER)CONTAINING_RECORD((Object), PH_OBJECT_HEADER, Body))
#else
FORCEINLINE
PPH_OBJECT_HEADER
NTAPI
PhObjectToObjectHeader(
    _In_ PVOID Object
    )
{
    return CONTAINING_RECORD(Object, PH_OBJECT_HEADER, Body);
}
#endif

/**
 * Gets a pointer to an object from an object header.
 *
 * \param ObjectHeader A pointer to an object header.
 *
 * \return A pointer to an object.
 */
#if defined(_PHLIB_)
#define PhObjectHeaderToObject(ObjectHeader) ((PVOID)&((PPH_OBJECT_HEADER)(ObjectHeader))->Body)
#else
FORCEINLINE
PVOID
PhObjectHeaderToObject(
    _In_ PPH_OBJECT_HEADER ObjectHeader
    )
{
    return &ObjectHeader->Body;
}
#endif

/**
 * Calculates the total size to allocate for an object.
 *
 * \param Size The size of the object to allocate.
 *
 * \return The new size, including space for the object header.
 */
#if defined(_PHLIB_)
#define PhAddObjectHeaderSize(Size) ((Size) + UFIELD_OFFSET(PH_OBJECT_HEADER, Body))
#else
FORCEINLINE
SIZE_T
PhAddObjectHeaderSize(
    _In_ SIZE_T Size
    )
{
    return UFIELD_OFFSET(PH_OBJECT_HEADER, Body) + Size;
}
#endif

/** An object type specifies a kind of object and its delete procedure. */
typedef struct _PH_OBJECT_TYPE
{
    /** The flags that were used to create the object type. */
    USHORT Flags;
    UCHAR TypeIndex;
    UCHAR Reserved;
    /** The total number of objects of this type that are alive. */
    ULONG NumberOfObjects;
    /** An optional procedure called when objects of this type are freed. */
    PPH_TYPE_DELETE_PROCEDURE DeleteProcedure;
    /** The name of the type. */
    PCWSTR Name;
    /** A free list to use when allocating for this type. */
    PH_FREE_LIST FreeList;
} PH_OBJECT_TYPE, *PPH_OBJECT_TYPE;

/**
 * Increments a reference count, but will never increment from a nonpositive value to 1.
 *
 * \param RefCount A pointer to a reference count.
 */
FORCEINLINE
BOOLEAN
PhpInterlockedIncrementSafe(
    _Inout_ PLONG RefCount
    )
{
    /* Here we will attempt to increment the reference count, making sure that it is positive. */
    return _InterlockedIncrementPositive(RefCount);
}

PPH_OBJECT_HEADER PhpAllocateObject(
    _In_ PPH_OBJECT_TYPE ObjectType,
    _In_ SIZE_T ObjectSize
    );

VOID PhpFreeObject(
    _In_ PPH_OBJECT_HEADER ObjectHeader
    );

VOID PhpDeferDeleteObject(
    _In_ PPH_OBJECT_HEADER ObjectHeader
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpDeferDeleteObjectRoutine(
    _In_ PVOID Parameter
    );

#endif

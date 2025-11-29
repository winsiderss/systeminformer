/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#pragma once

#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4324)

typedef struct _KPH_RING_HEADER
{
    union
    {
        struct
        {
            ULONG64 Length : 30;
            ULONG64 Busy : 1;
            ULONG64 Discard : 1;
            ULONG64 Reset : 1;
            ULONG64 Alignment : 4;
            ULONG64 Spare : 27;
        };

        ULONG64 Value;
    };

#ifdef _WIN64
    ULONG64 Padding;
#endif
} KPH_RING_HEADER, *PKPH_RING_HEADER;

C_ASSERT(sizeof(KPH_RING_HEADER) == MEMORY_ALLOCATION_ALIGNMENT);

#define KPH_RING_BUFFER_HEADER_SIZE MEMORY_ALLOCATION_ALIGNMENT
#define KPH_RING_BUFFER_RESERVE_MAX (((1UL << 30) - 1) -                       \
                                     (MEMORY_ALLOCATION_ALIGNMENT - 1) -       \
                                     (KPH_RING_BUFFER_HEADER_SIZE * 2))

typedef struct _KPH_RING_CONSUMER_BLOCK
{
    ULONG Position;
} KPH_RING_CONSUMER_BLOCK, *PKPH_RING_CONSUMER_BLOCK;

typedef struct _KPH_RING_PRODUCER_BLOCK
{
    ULONG Position;
    ULONG Length;
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) BYTE Buffer[ANYSIZE_ARRAY];
} KPH_RING_PRODUCER_BLOCK, *PKPH_RING_PRODUCER_BLOCK;

typedef struct _KPH_RING_BUFFER_USER
{
    PKPH_RING_CONSUMER_BLOCK Consumer;
    PKPH_RING_PRODUCER_BLOCK Producer;
} KPH_RING_BUFFER_USER, *PKPH_RING_BUFFER_USER;

typedef
_Function_class_(KPH_RING_CALLBACK)
BOOLEAN
NTAPI
KPH_RING_CALLBACK(
    _In_opt_ PVOID Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );
typedef KPH_RING_CALLBACK* PKPH_RING_CALLBACK;

VOID KphProcessRingBuffer(
    _In_ PKPH_RING_BUFFER_USER Ring,
    _In_ PKPH_RING_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

typedef struct _KPH_RING_BUFFER_CONNECT
{
    _In_ ULONG Length;
    _In_opt_ HANDLE EventHandle;
    _Out_ KPH_RING_BUFFER_USER Ring;
} KPH_RING_BUFFER_CONNECT, *PKPH_RING_BUFFER_CONNECT;

#pragma warning(pop)

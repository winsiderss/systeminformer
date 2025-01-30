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

#include <kphlibbase.h>
#include <kphringbuff.h>

#ifndef _KERNEL_MODE
#include <ntintsafe.h>
#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif
#endif

/**
 * \brief Processes the ring buffer.
 *
 * \param[in] Ring Pointer to the user ring buffer block.
 * \param[in] Callback Routine to process the ring buffer entries. The routine
 * should return FALSE to continue to processing, otherwise TRUE to stop.
 * \param[in] Context The context to pass to the callback routine.
 */
VOID KphProcessRingBuffer(
    _In_ PKPH_RING_BUFFER_USER Ring,
    _In_ PKPH_RING_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    ULONG consumerPos;

    consumerPos = ReadULongAcquire(&Ring->Consumer->Position);

    for (BOOLEAN done = FALSE; !done; NOTHING)
    {
        ULONG producerPos;
        PKPH_RING_HEADER headerPointer;
        KPH_RING_HEADER header;
        PVOID buffer;

        producerPos = ReadULongAcquire(&Ring->Producer->Position);

        if (consumerPos == producerPos)
        {
            done = Callback(Context, NULL, 0);
            continue;
        }

        headerPointer = Add2Ptr(Ring->Producer->Buffer, consumerPos);

        header.Value = ReadULong64Acquire(&headerPointer->Value);

        if (header.Reset)
        {
            //
            // Producer wrapped, reset to start.
            //
            WriteULongRelease(&Ring->Consumer->Position, 0);
            consumerPos = 0;
            continue;
        }

        //
        // Check if the producer is busy at this position. If it is then it
        // hasn't committed or discarded this region yet.
        //
        if (header.Busy)
        {
            done = Callback(Context, NULL, 0);
            continue;
        }

        consumerPos += KPH_RING_BUFFER_HEADER_SIZE;
        consumerPos += (ULONG)header.Length;
        consumerPos += (ULONG)header.Alignment;

        if (!header.Discard)
        {
            buffer = Add2Ptr(headerPointer, KPH_RING_BUFFER_HEADER_SIZE);

            done = Callback(Context, buffer, (ULONG)header.Length);
        }

        WriteULongRelease(&Ring->Consumer->Position, consumerPos);
    }
}

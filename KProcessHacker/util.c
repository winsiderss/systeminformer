/*
 * Process Hacker Driver - 
 *   utility functions
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

#include "include/kph.h"

/* KphInitializeStream
 * 
 * Initializes a stream.
 * 
 * Stream: The stream to initialize.
 * Buffer: The buffer to use.
 * Length: The maximum number of bytes that can be stored in 
 * the buffer. If an attempt is made to overrun or underrun 
 * the buffer, an exception will be raised.
 */
VOID KphInitializeStream(
    __out PKPH_STREAM Stream,
    __in PVOID Buffer,
    __in ULONG Length
    )
{
    ASSERT(Length > 0);
    
    Stream->Buffer = Buffer;
    Stream->Length = Length;
    Stream->Position = 0;
}

/* KphSeekStream
 * 
 * Changes the position of a stream.
 */
ULONG KphSeekStream(
    __inout PKPH_STREAM Stream,
    __in LONG Offset,
    __in KPH_STREAM_ORIGIN Origin
    )
{
    ULONG newPosition;
    
    switch (Origin)
    {
        case StartOrigin:
        {
            /* Can't seek to before the start of the buffer. */
            if (Offset < 0)
                ExRaiseStatus(STATUS_INVALID_PARAMETER_2);
            
            newPosition = Offset;
        }
        break;
        
        case CurrentOrigin:
        {
            newPosition = Stream->Position + Offset;
        }
        break;
        
        case EndOrigin:
        {
            newPosition = Stream->Length - Offset - 1;
        }
        break;
    }
    
    /* Check the new position and raise an exception if 
     * appropriate.
     */
    KphCheckStreamPosition(Stream, newPosition);
    Stream->Position = newPosition;
    
    return newPosition;
}

/* KphWriteDataStream
 * 
 * Writes data to a stream.
 */
ULONG KphWriteDataStream(
    __inout PKPH_STREAM Stream,
    __in PVOID Data,
    __in ULONG Length
    )
{
    /* Check if we are going to overrun the buffer. */
    KphCheckStreamPosition(Stream, Stream->Position + Length);
    /* Copy the data. */
    memcpy(
        PTR_ADD_OFFSET(Stream->Buffer, Stream->Position),
        Data,
        Length
        );
    
    /* Increase the position. */
    return Stream->Position += Length;
}

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

#ifndef _UTIL_H
#define _UTIL_H

/* Streams
 * 
 * Streams are small buffer management structures. They 
 * automatically raise an exception if the buffer is overrun.
 */

typedef struct _KPH_STREAM
{
    PVOID Buffer;
    ULONG Length;
    ULONG Position;
} KPH_STREAM, *PKPH_STREAM;

typedef enum _KPH_STREAM_ORIGIN
{
    StartOrigin,
    CurrentOrigin,
    EndOrigin
} KPH_STREAM_ORIGIN;

VOID KphInitializeStream(
    __out PKPH_STREAM Stream,
    __in PVOID Buffer,
    __in ULONG Length
    );

ULONG KphWriteDataStream(
    __inout PKPH_STREAM Stream,
    __in PVOID Data,
    __in ULONG Length
    );

/* KphCheckStreamPosition
 * 
 * Checks a stream position and raises an exception if 
 * appropriate.
 */
FORCEINLINE VOID KphCheckStreamPosition(
    __in PKPH_STREAM Stream,
    __in ULONG Position
    )
{
    if (Position > Stream->Length)
        ExRaiseStatus(STATUS_BUFFER_TOO_SMALL);
}

/* KphPositionStream
 * 
 * Gets the current position of the specified stream.
 */
FORCEINLINE ULONG KphPositionStream(
    __in PKPH_STREAM Stream
    )
{
    return Stream->Position;
}

/* KphWriteInt8Stream
 * 
 * Writes a 1-byte value to a stream.
 */
FORCEINLINE VOID KphWriteInt8Stream(
    __inout PKPH_STREAM Stream,
    __in BOOLEAN Value
    )
{
    KphWriteDataStream(Stream, &Value, sizeof(BOOLEAN));
}

/* KphWriteInt16Stream
 * 
 * Writes a 2-byte value to a stream.
 */
FORCEINLINE VOID KphWriteInt16Stream(
    __inout PKPH_STREAM Stream,
    __in SHORT Value
    )
{
    KphWriteDataStream(Stream, &Value, sizeof(SHORT));
}

/* KphWriteInt32Stream
 * 
 * Writes a 4-byte value to a stream.
 */
FORCEINLINE VOID KphWriteInt32Stream(
    __inout PKPH_STREAM Stream,
    __in LONG Value
    )
{
    KphWriteDataStream(Stream, &Value, sizeof(LONG));
}

/* KphWriteInt64Stream
 * 
 * Writes a 8-byte value to a stream.
 */
FORCEINLINE VOID KphWriteInt64Stream(
    __inout PKPH_STREAM Stream,
    __in PLARGE_INTEGER Value
    )
{
    KphWriteDataStream(Stream, Value, sizeof(LARGE_INTEGER));
}

#endif

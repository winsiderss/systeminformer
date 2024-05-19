/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#ifdef T

#include <templ.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct T___(_PH_CIRCULAR_BUFFER, T)
{
    ULONG Size;
#ifdef PH_CIRCULAR_BUFFER_POWER_OF_TWO_SIZE
    ULONG SizeMinusOne;
#endif
    ULONG Count;
    LONG Index;
    T *Data;
} T___(PH_CIRCULAR_BUFFER, T), *T___(PPH_CIRCULAR_BUFFER, T);

PHLIBAPI
VOID
NTAPI
T___(PhInitializeCircularBuffer, T)(
    _Out_ T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    _In_ ULONG Size
    );

PHLIBAPI
VOID
NTAPI
T___(PhDeleteCircularBuffer, T)(
    _Inout_ T___(PPH_CIRCULAR_BUFFER, T) Buffer
    );

PHLIBAPI
VOID
NTAPI
T___(PhResizeCircularBuffer, T)(
    _Inout_ T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    _In_ ULONG NewSize
    );

PHLIBAPI
VOID
NTAPI
T___(PhClearCircularBuffer, T)(
    _Inout_ T___(PPH_CIRCULAR_BUFFER, T) Buffer
    );

PHLIBAPI
VOID
NTAPI
T___(PhCopyCircularBuffer, T)(
    _Inout_ T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    _Out_writes_(Count) T *Destination,
    _In_ ULONG Count
    );

FORCEINLINE T T___(PhGetItemCircularBuffer, T)(
    _In_ T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    _In_ LONG Index
    )
{
#ifdef PH_CIRCULAR_BUFFER_POWER_OF_TWO_SIZE
    return Buffer->Data[(Buffer->Index + Index) & Buffer->SizeMinusOne];
#else
    ULONG size;

    size = Buffer->Size;
    // Modulo is dividend-based.
    return Buffer->Data[(((Buffer->Index + Index) % size) + size) % size];
#endif
}

FORCEINLINE VOID T___(PhSetItemCircularBuffer, T)(
    _Inout_ T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    _In_ LONG Index,
    _In_ T Value
    )
{
#ifdef PH_CIRCULAR_BUFFER_POWER_OF_TWO_SIZE
    Buffer->Data[(Buffer->Index + Index) & Buffer->SizeMinusOne] = Value;
#else
    ULONG size;

    size = Buffer->Size;
    Buffer->Data[(((Buffer->Index + Index) % size) + size) % size] = Value;
#endif
}

FORCEINLINE VOID T___(PhAddItemCircularBuffer, T)(
    _Inout_ T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    _In_ T Value
    )
{
#ifdef PH_CIRCULAR_BUFFER_POWER_OF_TWO_SIZE
    Buffer->Data[Buffer->Index = ((Buffer->Index - 1) & Buffer->SizeMinusOne)] = Value;
#else
    ULONG size;

    size = Buffer->Size;
    Buffer->Data[Buffer->Index = (((Buffer->Index - 1) % size) + size) % size] = Value;
#endif

    if (Buffer->Count < Buffer->Size)
        Buffer->Count++;
}

FORCEINLINE T T___(PhAddItemCircularBuffer2, T)(
    _Inout_ T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    _In_ T Value
    )
{
    LONG index;
    T oldValue;

#ifdef PH_CIRCULAR_BUFFER_POWER_OF_TWO_SIZE
    index = ((Buffer->Index - 1) & Buffer->SizeMinusOne);
#else
    ULONG size;

    size = Buffer->Size;
    index = (((Buffer->Index - 1) % size) + size) % size;
#endif

    Buffer->Index = index;
    oldValue = Buffer->Data[index];
    Buffer->Data[index] = Value;

    if (Buffer->Count < Buffer->Size)
        Buffer->Count++;

    return oldValue;
}

#ifdef __cplusplus
}
#endif

#endif

#ifdef T

#include <templ.h>

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
    __out T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    __in ULONG Size
    );

PHLIBAPI
VOID
NTAPI
T___(PhDeleteCircularBuffer, T)(
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer
    );

PHLIBAPI
VOID
NTAPI
T___(PhResizeCircularBuffer, T)(
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    __in ULONG NewSize
    );

PHLIBAPI
VOID
NTAPI
T___(PhClearCircularBuffer, T)(
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer
    );

PHLIBAPI
VOID
NTAPI
T___(PhCopyCircularBuffer, T)(
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    __out T *Destination,
    __in ULONG Count
    );

FORCEINLINE T T___(PhGetItemCircularBuffer, T)(
    __in T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    __in LONG Index
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
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    __in LONG Index,
    __in T Value
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
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    __in T Value
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
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    __in T Value
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

#endif

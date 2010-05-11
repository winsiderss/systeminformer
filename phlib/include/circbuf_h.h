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

VOID T___(PhInitializeCircularBuffer, T)(
    __out T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    __in ULONG Size
    );

VOID T___(PhDeleteCircularBuffer, T)(
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer
    );

VOID T___(PhResizeCircularBuffer, T)(
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    __in ULONG NewSize
    );

VOID T___(PhClearCircularBuffer, T)(
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer
    );

VOID T___(PhCopyCircularBuffer, T)(
    __inout T___(PPH_CIRCULAR_BUFFER, T) Buffer,
    __out T *Destination,
    __in ULONG Count
    );

FORCEINLINE T T___(PhCircularBufferGet, T)(
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

FORCEINLINE VOID T___(PhCircularBufferSet, T)(
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

FORCEINLINE VOID T___(PhCircularBufferAdd, T)(
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
}

#endif

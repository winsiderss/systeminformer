#define BASESUP_PRIVATE
#include <phbase.h>

PPH_OBJECT_TYPE PhStringType;

BOOLEAN PhInitializeBase()
{
    return NT_SUCCESS(PhCreateObjectType(
        &PhStringType,
        0,
        NULL
        ));
}

PVOID PhAllocate(
    __in SIZE_T Size
    )
{
    return RtlAllocateHeap(PhHeapHandle, 0, Size);
}

VOID PhFree(
    __in PVOID Memory
    )
{
    RtlFreeHeap(PhHeapHandle, 0, Memory);
}

PVOID PhReAlloc(
    __in PVOID Memory,
    __in SIZE_T Size
    )
{
    return RtlReAllocateHeap(PhHeapHandle, 0, Memory, Size);
}

PPH_STRING PhCreateString(
    __in PWSTR Buffer
    )
{
    return PhCreateStringEx(Buffer, wcslen(Buffer) * sizeof(WCHAR));
}

PPH_STRING PhCreateStringEx(
    __in PWSTR Buffer,
    __in SIZE_T Length
    )
{
    PPH_STRING string;

    if (!NT_SUCCESS(PhCreateObject(
        &string,
        FIELD_OFFSET(PH_STRING, Buffer) + Length + sizeof(WCHAR), // null terminator
        0,
        PhStringType,
        0
        )))
        return NULL;

    string->us.MaximumLength = string->us.Length = (USHORT)Length;
    string->us.Buffer = string->Buffer;
    memcpy(string->Buffer, Buffer, Length);
    string->Buffer[Length / sizeof(WCHAR)] = 0;

    return string;
}

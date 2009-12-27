#define BASESUP_PRIVATE
#include <phbase.h>

VOID PhpListDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

PPH_OBJECT_TYPE PhStringType;
PPH_OBJECT_TYPE PhListType;

BOOLEAN PhInitializeBase()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhStringType,
        0,
        NULL
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhListType,
        0,
        PhpListDeleteProcedure
        )))
        return FALSE;

    return TRUE;
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
    __in_opt PWSTR Buffer,
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
    string->Buffer[Length / sizeof(WCHAR)] = 0;

    if (Buffer)
    {
        memcpy(string->Buffer, Buffer, Length);
    }

    return string;
}

PPH_LIST PhCreateList(
    __in ULONG InitialCapacity
    )
{
    PPH_LIST list;

    if (!NT_SUCCESS(PhCreateObject(
        &list,
        sizeof(PH_LIST),
        0,
        PhListType,
        0
        )))
        return NULL;

    list->Count = 0;
    list->AllocatedCount = InitialCapacity;
    list->Items = PhAllocate(list->AllocatedCount * sizeof(PVOID));

    return list;
}

VOID PhpListDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_LIST list = (PPH_LIST)Object;
    ULONG i;

    for (i = 0; i < list->Count; i++)
        PhDereferenceObject(list->Items[i]);
}

VOID PhAddListItem(
    __inout PPH_LIST List,
    __in PVOID Item
    )
{
    if (List->Count == List->AllocatedCount)
    {
        List->AllocatedCount *= 2;
        List->Items = PhReAlloc(List->Items, List->AllocatedCount * sizeof(PVOID));
    }

    PhReferenceObject(Item);
    List->Items[List->Count++] = Item;
}

VOID PhClearList(
    __inout PPH_LIST List
    )
{
    ULONG i;

    for (i = 0; i < List->Count; i++)
        PhDereferenceObject(List->Items[i]);

    List->Count = 0;
}

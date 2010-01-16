/*
 * Process Hacker - 
 *   base support functions
 * 
 * Copyright (C) 2009-2010 wj32
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

#define BASESUP_PRIVATE
#include <phbase.h>
#include "math.h"

VOID NTAPI PhpStringBuilderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

VOID NTAPI PhpListDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

VOID NTAPI PhpQueueDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

VOID NTAPI PhpHashtableDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

PPH_OBJECT_TYPE PhStringType;
PPH_OBJECT_TYPE PhAnsiStringType;
PPH_OBJECT_TYPE PhStringBuilderType;
PPH_OBJECT_TYPE PhListType;
PPH_OBJECT_TYPE PhQueueType;
PPH_OBJECT_TYPE PhHashtableType;

static ULONG PhpPrimeNumbers[] =
{
    0x3, 0x7, 0xb, 0x11, 0x17, 0x1d, 0x25, 0x2f, 0x3b, 0x47, 0x59, 0x6b, 0x83,
    0xa3, 0xc5, 0xef, 0x125, 0x161, 0x1af, 0x209, 0x277, 0x2f9, 0x397, 0x44f,
    0x52f, 0x63d, 0x78b, 0x91d, 0xaf1, 0xd2b, 0xfd1, 0x12fd, 0x16cf, 0x1b65,
    0x20e3, 0x2777, 0x2f6f, 0x38ff, 0x446f, 0x521f, 0x628d, 0x7655, 0x8e01, 
    0xaa6b, 0xcc89, 0xf583, 0x126a7, 0x1619b, 0x1a857, 0x1fd3b, 0x26315, 0x2dd67,
    0x3701b, 0x42023, 0x4f361, 0x5f0ed, 0x72125, 0x88e31, 0xa443b, 0xc51eb,
    0xec8c1, 0x11bdbf, 0x154a3f, 0x198c4f, 0x1ea867, 0x24ca19, 0x2c25c1, 0x34fa1b,
    0x3f928f, 0x4c4987, 0x5b8b6f, 0x6dda89
};

/**
 * Initializes the base support module.
 */
BOOLEAN PhInitializeBase()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhStringType,
        0,
        NULL
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhAnsiStringType,
        0,
        NULL
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhStringBuilderType,
        0,
        PhpStringBuilderDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhListType,
        0,
        PhpListDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhQueueType,
        0,
        PhpQueueDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhHashtableType,
        0,
        PhpHashtableDeleteProcedure
        )))
        return FALSE;

    PhWorkQueueInitialization();

    return TRUE;
}

/**
 * Allocates a block of memory.
 *
 * \param Size The number of bytes to allocate.
 *
 * \return A pointer to the allocated block of 
 * memory.
 *
 * \remarks If the function fails to allocate 
 * the block of memory, it raises a 
 * STATUS_INSUFFICIENT_RESOURCES exception.
 */
PVOID PhAllocate(
    __in SIZE_T Size
    )
{
    PVOID memory;

    memory = RtlAllocateHeap(PhHeapHandle, 0, Size);

    if (!memory)
        PhRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);

    return memory;
}

/**
 * Frees a block of memory allocated with 
 * PhAllocate().
 *
 * \param Memory A pointer to a block of memory.
 */
VOID PhFree(
    __in PVOID Memory
    )
{
    RtlFreeHeap(PhHeapHandle, 0, Memory);
}

/**
 * Re-allocates a block of memory originally 
 * allocated with PhAllocate().
 *
 * \param Memory A pointer to a block of memory.
 * \param Size The new size of the memory block, 
 * in bytes.
 *
 * \return A pointer to the new block of memory. 
 * The existing contents of the memory block are 
 * copied to the new block.
 */
PVOID PhReAlloc(
    __in PVOID Memory,
    __in SIZE_T Size
    )
{
    return RtlReAllocateHeap(PhHeapHandle, 0, Memory, Size);
}

/**
 * Initializes an event object.
 *
 * \param Event A pointer to an event object.
 */
VOID PhInitializeEvent(
    __out PPH_EVENT Event
    )
{
    Event->Value = PH_EVENT_REFCOUNT_INC;
    Event->EventHandle = NULL;
}

FORCEINLINE VOID PhpDereferenceEvent(
    __inout PPH_EVENT Event
    )
{
    ULONG value;

    value = _InterlockedExchangeAdd(&Event->Value, -PH_EVENT_REFCOUNT_INC) - PH_EVENT_REFCOUNT_INC;

    // See if the reference count has become 0.
    if ((value >> PH_EVENT_REFCOUNT_SHIFT) == 0)
    {
        if (Event->EventHandle)
        {
            CloseHandle(Event->EventHandle);
            Event->EventHandle = NULL;
        }
    }
}

FORCEINLINE VOID PhpReferenceEvent(
    __inout PPH_EVENT Event
    )
{
    _InterlockedExchangeAdd(&Event->Value, PH_EVENT_REFCOUNT_INC);
}

/**
 * Sets an event object.
 * Any threads waiting on the event will be released.
 *
 * \param Event A pointer to an event object.
 *
 * \remarks This function is thread-safe with regards to
 * calls to PhSetEvent() and PhWaitForEvent().
 */
VOID PhSetEvent(
    __inout PPH_EVENT Event
    )
{
    ULONG value;
    HANDLE eventHandle;

    // Try to set the bit.
    do
    {
        value = Event->Value;

        // Has the event already been set?
        if (value & PH_EVENT_SET)
            return;
    } while (_InterlockedCompareExchange(
        &Event->Value,
        value + PH_EVENT_SET,
        value
        ) != value);

    // Do an up-to-date read.
    eventHandle = *(volatile HANDLE *)(&Event->EventHandle);

    if (eventHandle)
    {
        SetEvent(eventHandle);
    }

    PhpDereferenceEvent(Event);
}

/**
 * Waits for an event object to be set.
 *
 * \param Event A pointer to an event object.
 * \param Timeout The timeout period, in milliseconds.
 *
 * \return TRUE if the event object was set before the 
 * timeout period expired, otherwise FALSE.
 *
 * \remarks This function is thread-safe with regards to
 * calls to PhSetEvent() and PhWaitForEvent().
 */
BOOLEAN PhWaitForEvent(
    __inout PPH_EVENT Event,
    __in ULONG Timeout
    )
{
    BOOLEAN result;
    ULONG value;
    HANDLE eventHandle;

    value = Event->Value;

    // Shortcut: if the event is set, return immediately.
    if (value & PH_EVENT_SET)
        return TRUE;

    // Shortcut: if the timeout is 0, return immediately 
    // if the event isn't set.
    if (Timeout == 0)
        return FALSE;

    // Prevent the event from being invalidated.
    PhpReferenceEvent(Event);

    eventHandle = *(volatile HANDLE *)(&Event->EventHandle);

    // Don't bother creating an event if we already have one.
    if (!eventHandle)
    {
        eventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);

        // Try to set the event handle to our event.
        if (_InterlockedCompareExchangePointer(
            &Event->EventHandle,
            eventHandle,
            NULL
            ) != NULL)
        {
            // Someone else set the event before we did.
            CloseHandle(eventHandle);
        }
    }

    // Essential: check the event one last time to see if 
    // it is set.
    if (!(*(volatile ULONG *)(&Event->Value) & PH_EVENT_SET))
    {
        result = WaitForSingleObject(Event->EventHandle, Timeout) == WAIT_OBJECT_0;
    }
    else
    {
        result = TRUE;
    }

    PhpDereferenceEvent(Event);

    return result;
}

/**
 * Resets an event's state.
 *
 * \param Event A pointer to an event object.
 *
 * \remarks This function is not thread-safe.
 * Make sure no other threads are using the 
 * event when you call this function.
 */
VOID PhResetEvent(
    __inout PPH_EVENT Event
    )
{
    if (PhTestEvent(Event))
        Event->Value = PH_EVENT_REFCOUNT_INC;
}

/**
 * Creates a string object from an existing 
 * null-terminated string.
 *
 * \param Buffer A null-terminated Unicode string.
 */
PPH_STRING PhCreateString(
    __in PWSTR Buffer
    )
{
    return PhCreateStringEx(Buffer, wcslen(Buffer) * sizeof(WCHAR));
}

/**
 * Creates a string object using a specified length.
 *
 * \param Buffer A null-terminated Unicode string.
 * \param Length The length, in bytes, of the string.
 */
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

/**
 * Creates a string object from an existing
 * null-terminated ANSI string.
 *
 * \param Buffer A null-terminated ANSI string.
 */
PPH_STRING PhCreateStringFromAnsi(
    __in PCHAR Buffer
    )
{
    return PhCreateStringFromAnsiEx(
        Buffer,
        strlen(Buffer)
        );
}

/**
 * Creates a string object from an existing
 * null-terminated ANSI string.
 *
 * \param Buffer A null-terminated ANSI string.
 * \param Length The number of bytes to use.
 */
PPH_STRING PhCreateStringFromAnsiEx(
    __in PCHAR Buffer,
    __in SIZE_T Length
    )
{
    NTSTATUS status;
    PPH_STRING string;
    ULONG unicodeBytes;

    status = RtlMultiByteToUnicodeSize(
        &unicodeBytes,
        Buffer,
        (ULONG)Length
        );

    if (!NT_SUCCESS(status))
        return NULL;

    string = PhCreateStringEx(NULL, unicodeBytes);

    status = RtlMultiByteToUnicodeN(
        string->Buffer,
        string->Length,
        NULL,
        Buffer,
        (ULONG)Length
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    return string;
}

/**
 * Concatenates multiple strings.
 *
 * \param Count The number of strings to concatenate.
 */
PPH_STRING PhConcatStrings(
    __in ULONG Count,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Count);

    return PhConcatStrings_V(Count, argptr);
}

/**
 * Concatenates multiple strings.
 *
 * \param Count The number of strings to concatenate.
 * \param ArgPtr A pointer to an array of strings.
 */
PPH_STRING PhConcatStrings_V(
    __in ULONG Count,
    __in va_list ArgPtr
    )
{
    va_list argptr;
    ULONG i;
    SIZE_T totalLength = 0;
    SIZE_T stringLength;
    PWSTR arg;
    PPH_STRING string;

    // Compute the total length, in bytes, of the strings.

    argptr = ArgPtr;

    for (i = 0; i < Count; i++)
    {
        arg = va_arg(argptr, PWSTR);
        totalLength += wcslen(arg) * sizeof(WCHAR);
    }

    // Create the string.

    string = PhCreateStringEx(NULL, totalLength);
    totalLength = 0;

    // Append the strings one by one.

    argptr = ArgPtr;

    for (i = 0; i < Count; i++)
    {
        arg = va_arg(argptr, PWSTR);
        stringLength = wcslen(arg) * sizeof(WCHAR);
        memcpy(
            &string->Buffer[totalLength / sizeof(WCHAR)],
            arg,
            stringLength
            );
        totalLength += stringLength;
    }

    return string;
}

/**
 * Creates a string using format specifiers.
 *
 * \param Format The format-control string.
 */
PPH_STRING PhFormatString(
    __in PWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);

    return PhFormatString_V(Format, argptr);
}

/**
 * Creates a string using format specifiers.
 *
 * \param Format The format-control string.
 * \param ArgPtr A pointer to the list of arguments.
 */
PPH_STRING PhFormatString_V(
    __in PWSTR Format,
    __in va_list ArgPtr
    )
{
    PPH_STRING string;
    INT length;

    length = _vscwprintf(Format, ArgPtr);

    if (length == -1)
        return NULL;

    string = PhCreateStringEx(NULL, length * sizeof(WCHAR));
    _vsnwprintf(string->Buffer, length, Format, ArgPtr);

    return string;
}

/**
 * Creates an ANSI string object from an existing 
 * null-terminated string.
 *
 * \param Buffer A null-terminated ANSI string.
 */
PPH_ANSI_STRING PhCreateAnsiString(
    __in PSTR Buffer
    )
{
    return PhCreateAnsiStringEx(Buffer, strlen(Buffer));
}

/**
 * Creates an ANSI string object using a specified length.
 *
 * \param Buffer A null-terminated ANSI string.
 * \param Length The length, in bytes, of the string.
 */
PPH_ANSI_STRING PhCreateAnsiStringEx(
    __in_opt PSTR Buffer,
    __in SIZE_T Length
    )
{
    PPH_ANSI_STRING string;

    if (!NT_SUCCESS(PhCreateObject(
        &string,
        FIELD_OFFSET(PH_ANSI_STRING, Buffer) + Length + sizeof(CHAR), // null terminator
        0,
        PhAnsiStringType,
        0
        )))
        return NULL;

    string->as.MaximumLength = string->as.Length = (USHORT)Length;
    string->as.Buffer = string->Buffer;
    string->Buffer[Length] = 0;

    if (Buffer)
    {
        memcpy(string->Buffer, Buffer, Length);
    }

    return string;
}

/**
 * Creates an ANSI string object from an existing
 * null-terminated Unicode string.
 *
 * \param Buffer A null-terminated Unicode string.
 */
PPH_ANSI_STRING PhCreateAnsiStringFromUnicode(
    __in PWSTR Buffer
    )
{
    return PhCreateAnsiStringFromUnicodeEx(
        Buffer,
        wcslen(Buffer) * sizeof(WCHAR)
        );
}

/**
 * Creates an ANSI string object from an existing
 * null-terminated Unicode string.
 *
 * \param Buffer A null-terminated Unicode string.
 * \param Length The number of bytes to use.
 */
PPH_ANSI_STRING PhCreateAnsiStringFromUnicodeEx(
    __in PWSTR Buffer,
    __in SIZE_T Length
    )
{
    NTSTATUS status;
    PPH_ANSI_STRING string;
    ULONG ansiBytes;

    status = RtlUnicodeToMultiByteSize(
        &ansiBytes,
        Buffer,
        (ULONG)Length
        );

    if (!NT_SUCCESS(status))
        return NULL;

    string = PhCreateAnsiStringEx(NULL, ansiBytes);

    status = RtlUnicodeToMultiByteN(
        string->Buffer,
        string->Length,
        NULL,
        Buffer,
        (ULONG)Length
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    return string;
}

/**
 * Creates a string builder object.
 *
 * \param InitialCapacity The number of bytes to allocate 
 * initially.
 */
PPH_STRING_BUILDER PhCreateStringBuilder(
    __in ULONG InitialCapacity
    )
{
    PPH_STRING_BUILDER stringBuilder;

    if (!NT_SUCCESS(PhCreateObject(
        &stringBuilder,
        sizeof(PH_STRING_BUILDER),
        0,
        PhStringBuilderType,
        0
        )))
        return NULL;

    stringBuilder->AllocatedLength = InitialCapacity;

    // Allocate a PH_STRING for the string builder. 
    // We will dereference it and allocate a new one 
    // when we need to resize the string.

    stringBuilder->String = PhCreateStringEx(
        NULL,
        stringBuilder->AllocatedLength
        );

    // We will keep modifying the Length field of the 
    // string so:
    // 1. We know how much of the string is used, and 
    // 2. The user can simply get a reference to the 
    //    string and use it as-is.

    stringBuilder->String->Length = 0;

    // Write the null terminator.
    stringBuilder->String->Buffer[0] = 0;

    return stringBuilder;
}

VOID NTAPI PhpStringBuilderDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_STRING_BUILDER stringBuilder = (PPH_STRING_BUILDER)Object;

    PhDereferenceObject(stringBuilder->String);
}

VOID PhpResizeStringBuilder(
    __in PPH_STRING_BUILDER StringBuilder,
    __in ULONG NewCapacity
    )
{
    PPH_STRING newString;

    // Double the string size. If that still isn't 
    // enough room, just use the new length.

    StringBuilder->AllocatedLength *= 2;

    if (StringBuilder->AllocatedLength < NewCapacity)
        StringBuilder->AllocatedLength = NewCapacity;

    // Allocate a new string.
    newString = PhCreateStringEx(NULL, StringBuilder->AllocatedLength);

    // Copy the old string to the new string.
    memcpy(
        newString->Buffer,
        StringBuilder->String->Buffer,
        StringBuilder->String->Length
        );

    // Copy the old string length.
    newString->Length = StringBuilder->String->Length;

    // Dereference the old string and replace it with 
    // the new string.
    PhDereferenceObject(StringBuilder->String);
    StringBuilder->String = newString;
}

FORCEINLINE VOID PhpWriteStringBuilderNullTerminator(
    __in PPH_STRING_BUILDER StringBuilder
    )
{
    StringBuilder->String->Buffer[StringBuilder->String->Length / sizeof(WCHAR)] = 0;
}

/**
 * Obtains a reference to the string constructed 
 * by a string builder object.
 *
 * \param StringBuilder A string builder object.
 *
 * \return A pointer to a string. You must free 
 * the string using PhDereferenceObject() when 
 * you no longer need it.
 *
 * \remarks Do not modify the string builder 
 * object after you have referenced the string. 
 * Otherwise, the string may change unexpectedly.
 */
PPH_STRING PhReferenceStringBuilderString(
    __in PPH_STRING_BUILDER StringBuilder
    )
{
    PPH_STRING string;

    string = StringBuilder->String;
    PhReferenceObject(string);

    return string;
}

/**
 * Appends a string to the end of a string builder 
 * string.
 *
 * \param StringBuilder A string builder object.
 * \param String The string to append.
 */
VOID PhStringBuilderAppend(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PPH_STRING String
    )
{
    PhStringBuilderAppendEx(
        StringBuilder,
        String->Buffer,
        String->Length
        );
}

/**
 * Appends a string to the end of a string builder 
 * string.
 *
 * \param StringBuilder A string builder object.
 * \param String The string to append.
 */
VOID PhStringBuilderAppend2(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PWSTR String
    )
{
    PhStringBuilderAppendEx(
        StringBuilder,
        String,
        (ULONG)wcslen(String) * sizeof(WCHAR)
        );
}

/**
 * Appends a string to the end of a string builder 
 * string.
 *
 * \param StringBuilder A string builder object.
 * \param String The string to append.
 * \param Length The number of bytes to append.
 */
VOID PhStringBuilderAppendEx(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PWSTR String,
    __in ULONG Length
    )
{
    if (Length == 0)
        return;

    // See if we need to re-allocate the string.
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + Length)
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + Length);
    }

    // Copy the string, add the length, then write the null terminator.

    memcpy(
        &StringBuilder->String->Buffer[StringBuilder->String->Length / sizeof(WCHAR)],
        String,
        Length
        );
    StringBuilder->String->Length += (USHORT)Length;
    PhpWriteStringBuilderNullTerminator(StringBuilder);
}

/**
 * Inserts a string into a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Index The index, in characters, at which to 
 * insert the string.
 * \param String The string to insert.
 */
VOID PhStringBuilderInsert(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG Index,
    __in PPH_STRING String
    )
{
    PhStringBuilderInsertEx(
        StringBuilder,
        Index,
        String->Buffer,
        String->Length
        );
}

/**
 * Inserts a string into a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Index The index, in characters, at which to 
 * insert the string.
 * \param String The string to insert.
 */
VOID PhStringBuilderInsert2(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG Index,
    __in PWSTR String
    )
{
    PhStringBuilderInsertEx(
        StringBuilder,
        Index,
        String,
        (ULONG)wcslen(String) * sizeof(WCHAR)
        );
}

/**
 * Inserts a string into a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Index The index, in characters, at which to 
 * insert the string.
 * \param String The string to insert.
 * \param Length The number of bytes to insert.
 */
VOID PhStringBuilderInsertEx(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG Index,
    __in PWSTR String,
    __in ULONG Length
    )
{
    if (Length == 0)
        return;

    // See if we need to re-allocate the string.
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + Length)
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + Length);
    }

    // Create some space for the string.
    memmove(
        &StringBuilder->String->Buffer[Index + Length / sizeof(WCHAR)],
        &StringBuilder->String->Buffer[Index],
        Length
        );

    // Copy the new string.
    memcpy(
        &StringBuilder->String->Buffer[Index],
        String,
        Length
        );

    PhpWriteStringBuilderNullTerminator(StringBuilder);
}

/**
 * Removes characters from a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param StartIndex The index, in characters, at 
 * which to begin removing characters.
 * \param Length The number of characters to remove.
 */
VOID PhStringBuilderRemove(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG StartIndex,
    __in ULONG Length
    )
{
    // Overwrite the removed part with the part 
    // behind it.

    memcpy(
        &StringBuilder->String->Buffer[StartIndex],
        &StringBuilder->String->Buffer[StartIndex + Length],
        StringBuilder->String->Length - (Length + StartIndex) * sizeof(WCHAR)
        );
    StringBuilder->String->Length -= (USHORT)(Length * sizeof(WCHAR));
    PhpWriteStringBuilderNullTerminator(StringBuilder);
}

/**
 * Creates a list object.
 *
 * \param InitialCapacity The number of elements to 
 * allocate storage for initially.
 */
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

    // Initial capacity of 0 is not allowed.
    if (InitialCapacity == 0)
        InitialCapacity = 1;

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

    PhFree(list->Items);
}

/**
 * Adds an item to a list.
 *
 * \param List A list object.
 * \param Item The item to add.
 */
VOID PhAddListItem(
    __inout PPH_LIST List,
    __in PVOID Item
    )
{
    // See if we need to resize the list.
    if (List->Count == List->AllocatedCount)
    {
        List->AllocatedCount *= 2;
        List->Items = PhReAlloc(List->Items, List->AllocatedCount * sizeof(PVOID));
    }

    List->Items[List->Count++] = Item;
}

/**
 * Clears a list.
 *
 * \param List A list object.
 */
VOID PhClearList(
    __inout PPH_LIST List
    )
{
    List->Count = 0;
}

/**
 * Locates an item in a list.
 *
 * \param List A list object.
 * \param Item The item to search for.
 *
 * \return The index of the item. If the 
 * item was not found, -1 is returned.
 */
ULONG PhIndexOfListItem(
    __in PPH_LIST List,
    __in PVOID Item
    )
{
    ULONG i;

    for (i = 0; i < List->Count; i++)
    {
        if (List->Items[i] == Item)
            return i;
    }

    return -1;
}

/**
 * Removes an item from a list.
 *
 * \param List A list object.
 * \param Index The index of the item.
 */
VOID PhRemoveListItem(
    __in PPH_LIST List,
    __in ULONG Index
    )
{
    PhRemoveListItems(List, Index, 1);
}

/**
 * Removes items from a list.
 *
 * \param List A list object.
 * \param StartIndex The index at which to begin 
 * removing items.
 * \param Count The number of items to remove.
 */
VOID PhRemoveListItems(
    __in PPH_LIST List,
    __in ULONG StartIndex,
    __in ULONG Count
    )
{
    // Shift the items after the items forward.
    memmove(
        &List->Items[StartIndex],
        &List->Items[StartIndex + Count],
        List->Count - StartIndex - Count
        );

    List->Count -= Count;
}

typedef struct _PH_LIST_QSORT_CONTEXT
{
    PPH_LIST_COMPARE_FUNCTION CompareFunction;
    PVOID Context;
} PH_LIST_QSORT_CONTEXT, *PPH_LIST_QSORT_CONTEXT;

static int __cdecl PhpListQSortCompare(
    __in void *context,
    __in const void *elem1,
    __in const void *elem2
    )
{
    PPH_LIST_QSORT_CONTEXT qsortContext = (PPH_LIST_QSORT_CONTEXT)context;

    return qsortContext->CompareFunction(
        *(PPVOID)elem1,
        *(PPVOID)elem2,
        qsortContext->Context
        );
}

/**
 * Sorts a list.
 *
 * \param List A list object.
 * \param CompareFunction A comparison function to 
 * compare two list items.
 * \param Context A user-defined value to pass to the 
 * comparison function.
 */
VOID PhSortList(
    __in PPH_LIST List,
    __in PPH_LIST_COMPARE_FUNCTION CompareFunction,
    __in PVOID Context
    )
{
    PH_LIST_QSORT_CONTEXT qsortContext;

    qsortContext.CompareFunction = CompareFunction;
    qsortContext.Context = Context;

    qsort_s(
        List->Items,
        List->Count,
        sizeof(PVOID),
        PhpListQSortCompare,
        &qsortContext
        );
}

/**
 * Creates a queue object.
 *
 * \param InitialCapacity The number of elements to 
 * allocate storage for initially.
 */
PPH_QUEUE PhCreateQueue(
    __in ULONG InitialCapacity
    )
{
    PPH_QUEUE queue;

    if (!NT_SUCCESS(PhCreateObject(
        &queue,
        sizeof(PH_QUEUE),
        0,
        PhQueueType,
        0
        )))
        return NULL;

    // Initial capacity of 0 is not allowed.
    if (InitialCapacity == 0)
        InitialCapacity = 1;

    queue->Count = 0;
    queue->AllocatedCount = InitialCapacity;
    queue->Items = PhAllocate(queue->AllocatedCount * sizeof(PVOID));
    queue->Head = 0;
    queue->Tail = 0;

    return queue;
}

VOID NTAPI PhpQueueDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_QUEUE queue = (PPH_QUEUE)Object;

    PhFree(queue->Items);
}

/**
 * Enqueues an item to a queue.
 *
 * \param Queue A queue object.
 * \param Item The item to enqueue.
 */
VOID PhEnqueueQueueItem(
    __inout PPH_QUEUE Queue,
    __in PVOID Item
    )
{
    // See if we need to resize the queue.
    if (Queue->Count == Queue->AllocatedCount)
    {
        ULONG oldAllocatedCount = Queue->AllocatedCount;
        PPVOID oldItems = Queue->Items;

        Queue->AllocatedCount *= 2;
        Queue->Items = PhAllocate(Queue->AllocatedCount * sizeof(PVOID));

        // Copy the old items over if necessary.
        if (Queue->Count > 0)
        {
            if (Queue->Head < Queue->Tail)
            {
                memcpy(Queue->Items, &oldItems[Queue->Head], Queue->Count * sizeof(PVOID));
            }
            else
            {
                memcpy(Queue->Items, &oldItems[Queue->Head], (oldAllocatedCount - Queue->Head) * sizeof(PVOID));
                memcpy(&Queue->Items[oldAllocatedCount - Queue->Head], oldItems, Queue->Tail * sizeof(PVOID));
            }
        }

        PhFree(oldItems);
        Queue->Head = 0;
        Queue->Tail = Queue->Count;
    }

    Queue->Items[Queue->Tail] = Item;
    Queue->Tail = (Queue->Tail + 1) % Queue->AllocatedCount;
    Queue->Count++;
}

/**
 * Dequeues an item from a queue.
 *
 * \param Queue A queue object.
 * \param Item A variable which receives the 
 * dequeued item.
 *
 * \return TRUE if an item was dequeued,
 * FALSE if the queue was empty.
 */
BOOLEAN PhDequeueQueueItem(
    __inout PPH_QUEUE Queue,
    __out PPVOID Item
    )
{
    if (Queue->Count == 0)
        return FALSE;

    *Item = Queue->Items[Queue->Head];
    Queue->Head = (Queue->Head + 1) % Queue->AllocatedCount;
    Queue->Count--;

    return TRUE;
}

/**
 * Retrieves an item from the front of a queue
 * without removing it.
 *
 * \param Queue A queue object.
 * \param Item A variable which receives the 
 * item.
 *
 * \return TRUE if an item was retrieved,
 * FALSE if the queue was empty.
 */
BOOLEAN PhPeekQueueItem(
    __in PPH_QUEUE Queue,
    __out PPVOID Item
    )
{
    if (Queue->Count == 0)
        return FALSE;

    *Item = Queue->Items[Queue->Head];

    return TRUE;
}

ULONG PhpGetPrimeNumber(
    __in ULONG Minimum
    )
{
    ULONG i, j;

    for (i = 0; i < sizeof(PhpPrimeNumbers) / sizeof(ULONG); i++)
    {
        if (PhpPrimeNumbers[i] >= Minimum)
            return PhpPrimeNumbers[i];
    }

    for (i = Minimum | 1; i < MAXLONG; i += 2)
    {
        ULONG sqrtI = (ULONG)sqrt(i);

        for (j = 3; j <= sqrtI; j += 2)
        {
            if (i % j == 0)
            {
                // Not a prime.
                continue;
            }
        }

        // Success.
        return i;
    }

    return Minimum;
}

/**
 * Creates a hashtable object.
 *
 * \param EntrySize The size of each hashtable entry, 
 * in bytes.
 * \param CompareFunction A comparison function that 
 * is executed to compare two hashtable entries.
 * \param HashFunction A hash function that is executed 
 * to generate a hash code for a hashtable entry.
 * \param InitialCapacity The number of entries to 
 * allocate storage for initially.
 */
PPH_HASHTABLE PhCreateHashtable(
    __in ULONG EntrySize,
    __in PPH_HASHTABLE_COMPARE_FUNCTION CompareFunction,
    __in PPH_HASHTABLE_HASH_FUNCTION HashFunction,
    __in ULONG InitialCapacity
    )
{
    PPH_HASHTABLE hashtable;

    if (!NT_SUCCESS(PhCreateObject(
        &hashtable,
        sizeof(PH_HASHTABLE),
        0,
        PhHashtableType,
        0
        )))
        return NULL;

    hashtable->EntrySize = EntrySize;
    hashtable->CompareFunction = CompareFunction;
    hashtable->HashFunction = HashFunction;

    // Allocate the buckets.
    hashtable->AllocatedBuckets = PhpGetPrimeNumber(InitialCapacity);
    hashtable->Buckets = PhAllocate(sizeof(ULONG) * hashtable->AllocatedBuckets);
    // Set all bucket values to -1.
    memset(hashtable->Buckets, 0xff, sizeof(ULONG) * hashtable->AllocatedBuckets);

    // Allocate the entries.
    hashtable->AllocatedEntries = hashtable->AllocatedBuckets;
    hashtable->Entries = PhAllocate(PH_HASHTABLE_ENTRY_SIZE(EntrySize) * hashtable->AllocatedEntries);

    hashtable->Count = 0;
    hashtable->FreeEntry = -1;
    hashtable->NextEntry = 0;

#ifdef PH_HASHTABLE_ENABLE_STATS
    hashtable->TotalCollisions = 0;
#endif

    return hashtable;
}

VOID PhpHashtableDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_HASHTABLE hashtable = (PPH_HASHTABLE)Object;

    PhFree(hashtable->Buckets);
    PhFree(hashtable->Entries);
}

FORCEINLINE ULONG PhpValidateHash(
    __in ULONG Hash
    )
{
#ifdef PH_HASHTABLE_ENABLE_FULL_HASH
    if (Hash != -1)
        return Hash;
    else
        return 0;
#else
    return Hash & MAXLONG;
#endif
}

VOID PhpResizeHashtable(
    __inout PPH_HASHTABLE Hashtable,
    __in ULONG NewCapacity
    )
{
    ULONG i;

    // Re-allocate the buckets. Note that we don't need to keep the 
    // contents.
    Hashtable->AllocatedBuckets = PhpGetPrimeNumber(NewCapacity);
    PhFree(Hashtable->Buckets);
    Hashtable->Buckets = PhAllocate(sizeof(ULONG) * Hashtable->AllocatedBuckets);
    // Set all bucket values to -1.
    memset(Hashtable->Buckets, 0xff, sizeof(ULONG) * Hashtable->AllocatedBuckets);

    // Re-allocate the entries.
    Hashtable->AllocatedEntries = Hashtable->AllocatedBuckets;
    Hashtable->Entries = PhReAlloc(
        Hashtable->Entries,
        PH_HASHTABLE_ENTRY_SIZE(Hashtable->EntrySize) * Hashtable->AllocatedEntries
        );

    // Re-distribute the entries among the buckets.
    for (i = 0; i < Hashtable->NextEntry; i++)
    {
        PPH_HASHTABLE_ENTRY entry = PH_HASHTABLE_GET_ENTRY(Hashtable, i);

        if (entry->HashCode != -1)
        {
            ULONG index = entry->HashCode % Hashtable->AllocatedBuckets;

            entry->Next = Hashtable->Buckets[index];
            Hashtable->Buckets[index] = i;
        }
    }
}

/**
 * Adds an entry to a hashtable.
 *
 * \param Hashtable A hashtable object.
 * \param Entry The entry to add.
 *
 * \return A pointer to the entry as stored in
 * the hashtable. This pointer is valid until 
 * the hashtable is modified. If the hashtable 
 * already contained an equal entry, NULL is returned.
 */
PVOID PhAddHashtableEntry(
    __inout PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    )
{
    ULONG hashCode; // hash code of the new entry
    ULONG index; // bucket index of the new entry
    ULONG freeEntry; // index of new entry in entry array 
    PPH_HASHTABLE_ENTRY entry; // pointer to new entry in entry array

    // Make sure the hashtable doesn't already contain the entry.
    if (PhGetHashtableEntry(Hashtable, Entry))
        return NULL;

    hashCode = PhpValidateHash(Hashtable->HashFunction(Entry));
    index = hashCode % Hashtable->AllocatedBuckets;

    // Use a free entry if possible.
    if (Hashtable->FreeEntry != -1)
    {
        freeEntry = Hashtable->FreeEntry;
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, freeEntry);
        Hashtable->FreeEntry = entry->Next;
    }
    else
    {
        // Use the next entry in the entry array.

        if (Hashtable->NextEntry == Hashtable->AllocatedEntries)
        {
            // Resize the hashtable.
            PhpResizeHashtable(Hashtable, Hashtable->AllocatedBuckets * 2);
            index = hashCode % Hashtable->AllocatedBuckets;
        }

        freeEntry = Hashtable->NextEntry++;
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, freeEntry);
    }

    // Initialize the entry.
    entry->HashCode = hashCode;
    entry->Next = Hashtable->Buckets[index];
    Hashtable->Buckets[index] = freeEntry;
    // Copy the user-supplied data to the entry.
    memcpy(&entry->Body, Entry, Hashtable->EntrySize);

    Hashtable->Count++;

#ifdef PH_HASHTABLE_ENABLE_STATS
    if (entry->Next != -1)
        Hashtable->TotalCollisions++;
#endif

    return &entry->Body;
}

/**
 * Clears a hashtable.
 *
 * \param Hashtable A hashtable object.
 */
VOID PhClearHashtable(
    __inout PPH_HASHTABLE Hashtable
    )
{
    if (Hashtable->Count > 0)
    {
        memset(Hashtable->Buckets, 0xff, sizeof(ULONG) * Hashtable->AllocatedBuckets);
        Hashtable->Count = 0;
        Hashtable->FreeEntry = -1;
        Hashtable->NextEntry = 0;
    }
}

/**
 * Enumerates the entries in a hashtable.
 *
 * \param Hashtable A hashtable object.
 * \param Entry A variable which receives a pointer 
 * to the hashtable entry. The pointer is valid 
 * until the hashtable is modified.
 * \param EnumerationKey A variable which is 
 * initialized to 0 before first calling this 
 * function.
 *
 * \return TRUE if an entry pointer was stored 
 * in \a Entry, FALSE if there are no more entries.
 *
 * \remarks Do not modify the hashtable while 
 * the hashtable is being enumerated (between calls 
 * to this function). Otherwise, the function may 
 * behave unexpectedly. You may reset the 
 * \a EnumerationKey variable to 0 if you wish to 
 * restart the enumeration.
 */
BOOLEAN PhEnumHashtable(
    __in PPH_HASHTABLE Hashtable,
    __out PPVOID Entry,
    __inout PULONG EnumerationKey
    )
{
    while (*EnumerationKey < Hashtable->NextEntry)
    {
        PPH_HASHTABLE_ENTRY entry = PH_HASHTABLE_GET_ENTRY(Hashtable, *EnumerationKey);

        (*EnumerationKey)++;

        if (entry->HashCode != -1)
        {
            *Entry = &entry->Body;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Locates an entry in a hashtable.
 *
 * \param Hashtable A hashtable object.
 * \param Entry An entry representing the 
 * entry to find.
 *
 * \return A pointer to the entry as stored in
 * the hashtable. This pointer is valid until 
 * the hashtable is modified. If the entry 
 * could not be found, NULL is returned.
 *
 * \remarks The entry specified in \a Entry 
 * can be a partial entry that is filled in enough
 * so that the comparison and hash functions can 
 * work with them.
 */
PVOID PhGetHashtableEntry(
    __in PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    )
{
    ULONG hashCode;
    ULONG i;
    PPH_HASHTABLE_ENTRY entry;

    hashCode = PhpValidateHash(Hashtable->HashFunction(Entry));
    i = Hashtable->Buckets[hashCode % Hashtable->AllocatedBuckets];

    for (; i != -1; i = entry->Next)
    {
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, i);

        if (entry->HashCode == hashCode && Hashtable->CompareFunction(&entry->Body, Entry))
        {
            return &entry->Body;
        }
    }

    return NULL;
}

/**
 * Removes an entry from a hashtable.
 *
 * \param Hashtable A hashtable object.
 * \param Entry The entry to remove.
 *
 * \return TRUE if the entry was removed, 
 * FALSE if the entry could not be found.
 *
 * \remarks The entry specified in \a Entry 
 * can be an actual entry pointer returned 
 * by PhGetHashtableEntry, or a partial 
 * entry.
 */
BOOLEAN PhRemoveHashtableEntry(
    __inout PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    )
{
    ULONG hashCode;
    ULONG index;
    ULONG i;
    ULONG previousIndex;
    PPH_HASHTABLE_ENTRY entry;

    hashCode = PhpValidateHash(Hashtable->HashFunction(Entry));
    index = hashCode % Hashtable->AllocatedBuckets;
    previousIndex = -1;

    for (i = Hashtable->Buckets[index]; i != -1; i = entry->Next)
    {
        entry = PH_HASHTABLE_GET_ENTRY(Hashtable, i);

        if (entry->HashCode == hashCode && Hashtable->CompareFunction(&entry->Body, Entry))
        {
            // Unlink the entry from the bucket.
            if (previousIndex == -1)
            {
                Hashtable->Buckets[index] = entry->Next;
            }
            else
            {
                PH_HASHTABLE_GET_ENTRY(Hashtable, previousIndex)->Next = entry->Next;
            }

            entry->HashCode = -1; // indicates the entry is not being used
            entry->Next = Hashtable->FreeEntry;
            Hashtable->FreeEntry = i;

            Hashtable->Count--;

            return TRUE;
        }

        previousIndex = i;
    }

    return FALSE;
}

/**
 * Generates a hash code for a sequence of bytes.
 *
 * \param Bytes A pointer to a byte array.
 * \param Length The number of bytes to hash.
 */
ULONG PhHashBytes(
    __in PUCHAR Bytes,
    __in ULONG Length
    )
{
    // Hsieh hash, http://www.azillionmonkeys.com/qed/hash.html
    ULONG hash;
    ULONG tmp;
    ULONG rem;

    if (!Length || !Bytes)
        return 0;

    hash = Length;
    rem = Length & 3;
    Length >>= 2;

    for (; Length > 0; Length--)
    {
        hash += *(PUSHORT)Bytes;
        tmp = (*(PUSHORT)(Bytes + 2) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        Bytes += 4;
        hash += hash >> 11;
    }

    switch (rem)
    {
    case 3:
        hash += *(PUSHORT)Bytes;
        hash ^= hash << 16;
        hash ^= Bytes[2] << 18;
        hash += hash >> 11;
        break;
    case 2:
        hash += *(PUSHORT)Bytes;
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1:
        hash += *Bytes;
        hash ^= hash << 10;
        hash += hash >> 1;
        break;
    }

    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

/**
 * Generates a hash code for a sequence of bytes.
 *
 * \param Bytes A pointer to a byte array.
 * \param Length The number of bytes to hash.
 */
ULONG PhHashBytesSdbm(
    __in PUCHAR Bytes,
    __in ULONG Length
    )
{
    ULONG hash = Length;
    PUCHAR endByte = Bytes + Length;

    while (Bytes != endByte)
    {
        hash = *Bytes + (hash << 6) + (hash << 16) - hash;
        Bytes++;
    }

    return hash;
}

/**
 * Initializes a callback object.
 *
 * \param Callback A pointer to a callback object.
 */
VOID PhInitializeCallback(
    __out PPH_CALLBACK Callback
    )
{
    InitializeListHead(&Callback->ListHead);
    PhInitializeFastLock(&Callback->ListLock);
}

/**
 * Frees resources used by a callback object.
 *
 * \param Callback A pointer to a callback object.
 */
VOID PhDeleteCallback(
    __inout PPH_CALLBACK Callback
    )
{
    PhDeleteFastLock(&Callback->ListLock);
}

/**
 * Registers a callback function to be notified.
 *
 * \param Callback A pointer to a callback object.
 * \param Function The callback function.
 * \param Context A user-defined value to pass to the 
 * callback function.
 * \param Registration A variable which receives 
 * registration information for the callback. Do not 
 * modify the contents of this structure and do not
 * free the storage for this structure until you have 
 * unregistered the callback.
 */
VOID PhRegisterCallback(
    __inout PPH_CALLBACK Callback,
    __in PPH_CALLBACK_FUNCTION Function,
    __in PVOID Context,
    __out PPH_CALLBACK_REGISTRATION Registration
    )
{
    Registration->Function = Function;
    Registration->Context = Context;
    Registration->Unregistering = FALSE;

    PhAcquireFastLockExclusive(&Callback->ListLock);
    InsertTailList(&Callback->ListHead, &Registration->ListEntry);
    PhReleaseFastLockExclusive(&Callback->ListLock);
}

/**
 * Unregisters a callback function.
 *
 * \param Callback A pointer to a callback object.
 * \param Registration The structure returned by 
 * PhRegisterCallback().
 */
VOID PhUnregisterCallback(
    __inout PPH_CALLBACK Callback,
    __inout PPH_CALLBACK_REGISTRATION Registration
    )
{
    Registration->Unregistering = TRUE;

    PhAcquireFastLockExclusive(&Callback->ListLock);
    RemoveEntryList(&Registration->ListEntry);
    PhReleaseFastLockExclusive(&Callback->ListLock);
}

/**
 * Notifies all registered callback functions.
 *
 * \param Callback A pointer to a callback object.
 * \param Parameter A value to pass to all callback 
 * functions.
 */
VOID PhInvokeCallback(
    __in PPH_CALLBACK Callback,
    __in PVOID Parameter
    )
{
    PLIST_ENTRY listEntry;

    PhAcquireFastLockShared(&Callback->ListLock);

    listEntry = Callback->ListHead.Flink;

    while (listEntry != &Callback->ListHead)
    {
        PPH_CALLBACK_REGISTRATION registration;

        registration = CONTAINING_RECORD(listEntry, PH_CALLBACK_REGISTRATION, ListEntry);
        listEntry = listEntry->Flink;

        if (registration->Unregistering)
            continue;

        PhReleaseFastLockShared(&Callback->ListLock);
        registration->Function(
            Parameter,
            registration->Context
            );
        PhAcquireFastLockShared(&Callback->ListLock);
    }

    PhReleaseFastLockShared(&Callback->ListLock);
}

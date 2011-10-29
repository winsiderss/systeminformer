/*
 * Process Hacker -
 *   base support functions
 *
 * Copyright (C) 2009-2011 wj32
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

/*
 * This file contains basic low-level code as well as general algorithmic code.
 *
 * Memory allocation. PhAllocate is a wrapper around RtlAllocateHeap, and always allocates
 * from the phlib heap. PhAllocatePage is a wrapper around NtAllocateVirtualMemory and allocates
 * pages.
 *
 * Null-terminated strings. The Ph*StringZ functions manipulate null-terminated strings. The
 * copying functions provide a simple way to copy strings which may not be null-terminated, but
 * have a specified limit.
 *
 * String. The design of the string object was chosen for maximum compatibility. As such each
 * string buffer must be null-terminated, and each object contains an embedded UNICODE_STRING
 * structure. Note that efficient sub-string creation (no copying, only references the parent
 * string object) could not be implemented due to the mandatory null-termination. String objects
 * must be regarded as immutable (for thread-safety reasons) unless the object has just been
 * created and no references have been shared.
 *
 * Full string. These strings use full SIZE_T length fields, bypassing the 32k size limit
 * imposed on normal string objects which use USHORT length fields. Since full string objects
 * are rare, they are mutable and have functions similar to those of string builders.
 *
 * String builder. This is a set of functions which allow for efficient modification of strings.
 * For performance reasons, these functions modify string objects directly, even though they are
 * normally immutable.
 *
 * List. A simple PVOID list that resizes itself when needed.
 *
 * Pointer list. Similar to the normal list object, but uses a free list in order to support
 * constant time insertion and deletion. In order for the free list to work, normal entries
 * have their lowest bit clear while free entries have their lowest bit set, with the index of
 * the next free entry in the upper bits.
 *
 * Hashtable. A hashtable with power-of-two bucket sizes and with all entries stored in a
 * single array. This improves locality but may be inefficient when resizing the hashtable. It
 * is a good idea to store pointers to objects as entries, as opposed to the objects themselves.
 *
 * Simple hashtable. A wrapper around the normal hashtable, with PVOID keys and PVOID values.
 *
 * Free list. A thread-safe memory allocation method where freed blocks are stored in a S-list,
 * and allocations are made from this list whenever possible.
 *
 * Callback. A thread-safe notification mechanism where clients can register callback functions
 * which are then invoked by other code.
 */

#define _PH_BASESUP_PRIVATE
#include <phbase.h>
#include <phintrnl.h>
#include <math.h>

typedef struct _PHP_BASE_THREAD_CONTEXT
{
    PUSER_THREAD_START_ROUTINE StartAddress;
    PVOID Parameter;
} PHP_BASE_THREAD_CONTEXT, *PPHP_BASE_THREAD_CONTEXT;

VOID NTAPI PhpFullStringDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

VOID NTAPI PhpListDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

VOID NTAPI PhpPointerListDeleteProcedure(
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

// Types

PPH_OBJECT_TYPE PhStringType;
PPH_OBJECT_TYPE PhAnsiStringType;
PPH_OBJECT_TYPE PhFullStringType;
PPH_OBJECT_TYPE PhListType;
PPH_OBJECT_TYPE PhPointerListType;
PPH_OBJECT_TYPE PhQueueType;
PPH_OBJECT_TYPE PhHashtableType;

// Misc.

static PPH_STRING PhSharedEmptyString = NULL;

// Threads

static PH_FREE_LIST PhpBaseThreadContextFreeList;
#ifdef DEBUG
ULONG PhDbgThreadDbgTlsIndex;
LIST_ENTRY PhDbgThreadListHead;
PH_QUEUED_LOCK PhDbgThreadListLock = PH_QUEUED_LOCK_INIT;
#endif

// Data

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
BOOLEAN PhInitializeBase(
    __in ULONG Flags
    )
{
    PH_OBJECT_TYPE_PARAMETERS parameters;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhStringType,
        L"String",
        0,
        NULL
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhAnsiStringType,
        L"AnsiString",
        0,
        NULL
        )))
        return FALSE;

    parameters.FreeListSize = sizeof(PH_FULL_STRING);
    parameters.FreeListCount = 32;

    if (!NT_SUCCESS(PhCreateObjectTypeEx(
        &PhFullStringType,
        L"FullString",
        PHOBJTYPE_USE_FREE_LIST,
        PhpFullStringDeleteProcedure,
        &parameters
        )))
        return FALSE;

    parameters.FreeListSize = sizeof(PH_LIST);
    parameters.FreeListCount = 128;

    if (!NT_SUCCESS(PhCreateObjectTypeEx(
        &PhListType,
        L"List",
        PHOBJTYPE_USE_FREE_LIST,
        PhpListDeleteProcedure,
        &parameters
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhPointerListType,
        L"PointerList",
        0,
        PhpPointerListDeleteProcedure
        )))
        return FALSE;

    if (!NT_SUCCESS(PhCreateObjectType(
        &PhQueueType,
        L"Queue",
        0,
        PhpQueueDeleteProcedure
        )))
        return FALSE;

    parameters.FreeListSize = sizeof(PH_HASHTABLE);
    parameters.FreeListCount = 64;

    if (!NT_SUCCESS(PhCreateObjectTypeEx(
        &PhHashtableType,
        L"Hashtable",
        PHOBJTYPE_USE_FREE_LIST,
        PhpHashtableDeleteProcedure,
        &parameters
        )))
        return FALSE;

    PhInitializeFreeList(&PhpBaseThreadContextFreeList, sizeof(PHP_BASE_THREAD_CONTEXT), 16);

#ifdef DEBUG
    PhDbgThreadDbgTlsIndex = TlsAlloc();
    InitializeListHead(&PhDbgThreadListHead);
#endif

    if (Flags & PHLIB_INIT_MODULE_WORK_QUEUE)
        PhWorkQueueInitialization();
    if (Flags & PHLIB_INIT_MODULE_HANDLE_TABLE)
        PhHandleTableInitialization();

    return TRUE;
}

NTSTATUS PhpBaseThreadStart(
    __in PVOID Parameter
    )
{
    NTSTATUS status;
    HRESULT result;
    PHP_BASE_THREAD_CONTEXT context;
#ifdef DEBUG
    PHP_BASE_THREAD_DBG dbg;
#endif

    context = *(PPHP_BASE_THREAD_CONTEXT)Parameter;
    PhFreeToFreeList(&PhpBaseThreadContextFreeList, Parameter);

#ifdef DEBUG
    dbg.ClientId = NtCurrentTeb()->ClientId;

    dbg.StartAddress = context.StartAddress;
    dbg.Parameter = context.Parameter;
    dbg.CurrentAutoPool = NULL;

    PhAcquireQueuedLockExclusive(&PhDbgThreadListLock);
    InsertTailList(&PhDbgThreadListHead, &dbg.ListEntry);
    PhReleaseQueuedLockExclusive(&PhDbgThreadListLock);

    TlsSetValue(PhDbgThreadDbgTlsIndex, &dbg);
#endif

    // Initialization code

    result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Call the user-supplied function.
    status = context.StartAddress(context.Parameter);

    // De-initialization code

    if (result == S_OK || result == S_FALSE)
        CoUninitialize();

#ifdef DEBUG
    PhAcquireQueuedLockExclusive(&PhDbgThreadListLock);
    RemoveEntryList(&dbg.ListEntry);
    PhReleaseQueuedLockExclusive(&PhDbgThreadListLock);
#endif

    return status;
}

/**
 * Creates a thread.
 *
 * \param StackSize The initial stack size of the thread.
 * \param StartAddress The function to execute in the thread.
 * \param Parameter A user-defined value to pass to the function.
 */
HANDLE PhCreateThread(
    __in_opt SIZE_T StackSize,
    __in PUSER_THREAD_START_ROUTINE StartAddress,
    __in_opt PVOID Parameter
    )
{
    HANDLE threadHandle;
    PPHP_BASE_THREAD_CONTEXT context;

    context = PhAllocateFromFreeList(&PhpBaseThreadContextFreeList);
    context->StartAddress = StartAddress;
    context->Parameter = Parameter;

    threadHandle = CreateThread(
        NULL,
        StackSize,
        PhpBaseThreadStart,
        context,
        0,
        NULL
        );

    if (threadHandle)
    {
        PHLIB_INC_STATISTIC(BaseThreadsCreated);
        return threadHandle;
    }
    else
    {
        PHLIB_INC_STATISTIC(BaseThreadsCreateFailed);
        PhFreeToFreeList(&PhpBaseThreadContextFreeList, context);
        return NULL;
    }
}

/**
 * Gets the current system time (UTC).
 *
 * \remarks Use this function instead of NtQuerySystemTime()
 * because no system calls are involved.
 */
VOID PhQuerySystemTime(
    __out PLARGE_INTEGER SystemTime
    )
{
    do
    {
        SystemTime->HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime->LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime->HighPart != USER_SHARED_DATA->SystemTime.High2Time);
}

/**
 * Gets the offset of the current time zone from UTC.
 */
VOID PhQueryTimeZoneBias(
    __out PLARGE_INTEGER TimeZoneBias
    )
{
    do
    {
        TimeZoneBias->HighPart = USER_SHARED_DATA->TimeZoneBias.High1Time;
        TimeZoneBias->LowPart = USER_SHARED_DATA->TimeZoneBias.LowPart;
    } while (TimeZoneBias->HighPart != USER_SHARED_DATA->TimeZoneBias.High2Time);
}

/**
 * Converts system time to local time.
 *
 * \param SystemTime A UTC time value.
 * \param LocalTime A variable which receives the local time value.
 * This may be the same variable as \a SystemTime.
 *
 * \remarks Use this function instead of RtlSystemTimeToLocalTime()
 * because no system calls are involved.
 */
VOID PhSystemTimeToLocalTime(
    __in PLARGE_INTEGER SystemTime,
    __out PLARGE_INTEGER LocalTime
    )
{
    LARGE_INTEGER timeZoneBias;

    PhQueryTimeZoneBias(&timeZoneBias);
    LocalTime->QuadPart = SystemTime->QuadPart - timeZoneBias.QuadPart;
}

/**
 * Converts local time to system time.
 *
 * \param LocalTime A local time value.
 * \param SystemTime A variable which receives the UTC time value.
 * This may be the same variable as \a LocalTime.
 *
 * \remarks Use this function instead of RtlLocalTimeToSystemTime()
 * because no system calls are involved.
 */
VOID PhLocalTimeToSystemTime(
    __in PLARGE_INTEGER LocalTime,
    __out PLARGE_INTEGER SystemTime
    )
{
    LARGE_INTEGER timeZoneBias;

    PhQueryTimeZoneBias(&timeZoneBias);
    SystemTime->QuadPart = LocalTime->QuadPart + timeZoneBias.QuadPart;
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
 * the block of memory, it raises an exception.
 * The block is guaranteed to be aligned at
 * MEMORY_ALLOCATION_ALIGNMENT bytes.
 */
__mayRaise
__checkReturn
__notnull
__bcount(Size)
PVOID PhAllocate(
    __in SIZE_T Size
    )
{
    return RtlAllocateHeap(PhHeapHandle, HEAP_GENERATE_EXCEPTIONS, Size);
}

/**
 * Allocates a block of memory.
 *
 * \param Size The number of bytes to allocate.
 *
 * \return A pointer to the allocated block of
 * memory, or NULL if the block could not be allocated.
 */
PVOID PhAllocateSafe(
    __in SIZE_T Size
    )
{
    return RtlAllocateHeap(PhHeapHandle, 0, Size);
}

/**
 * Allocates a block of memory.
 *
 * \param Size The number of bytes to allocate.
 * \param Flags Flags controlling the allocation.
 *
 * \return A pointer to the allocated block of
 * memory, or NULL if the block could not be allocated.
 */
PVOID PhAllocateExSafe(
    __in SIZE_T Size,
    __in ULONG Flags
    )
{
    return RtlAllocateHeap(PhHeapHandle, Flags, Size);
}

/**
 * Frees a block of memory allocated with
 * PhAllocate().
 *
 * \param Memory A pointer to a block of memory.
 */
VOID PhFree(
    __in __post_invalid PVOID Memory
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
 *
 * \remarks If the function fails to allocate
 * the block of memory, it raises an exception.
 */
__mayRaise PVOID PhReAllocate(
    __in PVOID Memory,
    __in SIZE_T Size
    )
{
    return RtlReAllocateHeap(PhHeapHandle, HEAP_GENERATE_EXCEPTIONS, Memory, Size);
}

/**
 * Re-allocates a block of memory originally
 * allocated with PhAllocate().
 *
 * \param Memory A pointer to a block of memory.
 * \param Size The new size of the memory block,
 * in bytes.
 *
 * \return A pointer to the new block of memory,
 * or NULL if the block could not be allocated.
 * The existing contents of the memory block are
 * copied to the new block.
 */
PVOID PhReAllocateSafe(
    __in PVOID Memory,
    __in SIZE_T Size
    )
{
    return RtlReAllocateHeap(PhHeapHandle, 0, Memory, Size);
}

/**
 * Allocates pages of memory.
 *
 * \param Size The number of bytes to allocate. The number of
 * pages allocated will be large enough to contain \a Size bytes.
 * \param NewSize The number of bytes actually allocated. This
 * is \a Size rounded up to the next multiple of PAGE_SIZE.
 *
 * \return A pointer to the allocated block of memory, or NULL
 * if the block could not be allocated.
 */
__checkReturn
__maybenull
PVOID PhAllocatePage(
    __in SIZE_T Size,
    __out_opt PSIZE_T NewSize
    )
{
    PVOID baseAddress;

    baseAddress = NULL;

    if (NT_SUCCESS(NtAllocateVirtualMemory(
        NtCurrentProcess(),
        &baseAddress,
        0,
        &Size,
        MEM_COMMIT,
        PAGE_READWRITE
        )))
    {
        if (NewSize)
            *NewSize = Size;

        return baseAddress;
    }
    else
    {
        return NULL;
    }
}

/**
 * Frees pages of memory allocated with PhAllocatePage().
 *
 * \param Memory A pointer to a block of memory.
 */
VOID PhFreePage(
    __in __post_invalid PVOID Memory
    )
{
    SIZE_T size;

    size = 0;

    NtFreeVirtualMemory(
        NtCurrentProcess(),
        &Memory,
        &size,
        MEM_RELEASE
        );
}

/**
 * Allocates space for and copies a string.
 *
 * \param String The string to duplicate.
 *
 * \return The new string, which can be freed using PhFree().
 */
PSTR PhDuplicateAnsiStringZ(
    __in PSTR String
    )
{
    PSTR newString;
    SIZE_T length;

    length = strlen(String) + 1; // include the null terminator

    newString = PhAllocate(length);
    memcpy(newString, String, length);

    return newString;
}

/**
 * Allocates space for and copies a string.
 *
 * \param String The string to duplicate.
 *
 * \return The new string, which can be freed using PhFree(), or
 * NULL if storage could not be allocated.
 */
PSTR PhDuplicateAnsiStringZSafe(
    __in PSTR String
    )
{
    PSTR newString;
    SIZE_T length;

    length = strlen(String) + 1; // include the null terminator

    newString = PhAllocateSafe(length);

    if (!newString)
        return NULL;

    memcpy(newString, String, length);

    return newString;
}

/**
 * Allocates space for and copies a string.
 *
 * \param String The string to duplicate.
 *
 * \return The new string, which can be freed using PhFree().
 */
PWSTR PhDuplicateUnicodeStringZ(
    __in PWSTR String
    )
{
    PWSTR newString;
    SIZE_T length;

    length = (wcslen(String) + 1) * sizeof(WCHAR); // include the null terminator

    newString = PhAllocate(length);
    memcpy(newString, String, length);

    return newString;
}

/**
 * Copies a string with optional null termination and
 * a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters
 * to copy, not including the null terminator. Specify
 * -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the
 * output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the
 * number of characters required to contain the input
 * string, including the null terminator. If the
 * function returns TRUE, this variable contains the
 * number of characters written to the output buffer.
 *
 * \return TRUE if the input string was copied to
 * the output string, otherwise FALSE.
 *
 * \remarks The function stops copying when it
 * encounters the first null character in the input
 * string. It will also stop copying when it reaches
 * the character count specified in \a InputCount, if
 * \a InputCount is not -1.
 */
BOOLEAN PhCopyAnsiStringZ(
    __in PSTR InputBuffer,
    __in ULONG InputCount,
    __out_ecount_z_opt(OutputCount) PSTR OutputBuffer,
    __in ULONG OutputCount,
    __out_opt PULONG ReturnCount
    )
{
    ULONG i;
    BOOLEAN copied;

    // Determine the length of the input string.

    if (InputCount != -1)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = (ULONG)strlen(InputBuffer);
    }

    // Copy the string if there is enough room.

    if (OutputBuffer && OutputCount >= i + 1) // need one character for null terminator
    {
        memcpy(OutputBuffer, InputBuffer, i);
        OutputBuffer[i] = 0;
        copied = TRUE;
    }
    else
    {
        copied = FALSE;
    }

    if (ReturnCount)
        *ReturnCount = i + 1;

    return copied;
}

/**
 * Copies a string with optional null termination and
 * a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters
 * to copy, not including the null terminator. Specify
 * -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the
 * output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the
 * number of characters required to contain the input
 * string, including the null terminator. If the
 * function returns TRUE, this variable contains the
 * number of characters written to the output buffer.
 *
 * \return TRUE if the input string was copied to
 * the output string, otherwise FALSE.
 *
 * \remarks The function stops copying when it
 * encounters the first null character in the input
 * string. It will also stop copying when it reaches
 * the character count specified in \a InputCount, if
 * \a InputCount is not -1.
 */
BOOLEAN PhCopyUnicodeStringZ(
    __in PWSTR InputBuffer,
    __in ULONG InputCount,
    __out_ecount_z_opt(OutputCount) PWSTR OutputBuffer,
    __in ULONG OutputCount,
    __out_opt PULONG ReturnCount
    )
{
    ULONG i;
    BOOLEAN copied;

    // Determine the length of the input string.

    if (InputCount != -1)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = (ULONG)wcslen(InputBuffer);
    }

    // Copy the string if there is enough room.

    if (OutputBuffer && OutputCount >= i + 1) // need one character for null terminator
    {
        memcpy(OutputBuffer, InputBuffer, i * sizeof(WCHAR));
        OutputBuffer[i] = 0;
        copied = TRUE;
    }
    else
    {
        copied = FALSE;
    }

    if (ReturnCount)
        *ReturnCount = i + 1;

    return copied;
}

/**
 * Copies a string with optional null termination and
 * a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters
 * to copy, not including the null terminator. Specify
 * -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the
 * output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the
 * number of characters required to contain the input
 * string, including the null terminator. If the
 * function returns TRUE, this variable contains the
 * number of characters written to the output buffer.
 *
 * \return TRUE if the input string was copied to
 * the output string, otherwise FALSE.
 *
 * \remarks The function stops copying when it
 * encounters the first null character in the input
 * string. It will also stop copying when it reaches
 * the character count specified in \a InputCount, if
 * \a InputCount is not -1.
 */
BOOLEAN PhCopyUnicodeStringZFromAnsi(
    __in PSTR InputBuffer,
    __in ULONG InputCount,
    __out_ecount_z_opt(OutputCount) PWSTR OutputBuffer,
    __in ULONG OutputCount,
    __out_opt PULONG ReturnCount
    )
{
    NTSTATUS status;
    ULONG i;
    ULONG unicodeBytes;
    BOOLEAN copied;

    // Determine the length of the input string.

    if (InputCount != -1)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = (ULONG)strlen(InputBuffer);
    }

    // Determine the length of the output string.

    status = RtlMultiByteToUnicodeSize(
        &unicodeBytes,
        InputBuffer,
        i
        );

    if (!NT_SUCCESS(status))
    {
        if (ReturnCount)
            *ReturnCount = -1;

        return FALSE;
    }

    // Convert the string to Unicode if there is enough room.

    if (OutputBuffer && OutputCount >= unicodeBytes / sizeof(WCHAR) + 1)
    {
        status = RtlMultiByteToUnicodeN(
            OutputBuffer,
            unicodeBytes,
            NULL,
            InputBuffer,
            i
            );

        if (NT_SUCCESS(status))
        {
            // RtlMultiByteToUnicodeN doesn't null terminate the string.
            *(PWCHAR)((PCHAR)OutputBuffer + unicodeBytes) = 0;
            copied = TRUE;
        }
        else
        {
            copied = FALSE;
        }
    }
    else
    {
        copied = FALSE;
    }

    if (ReturnCount)
        *ReturnCount = unicodeBytes / sizeof(WCHAR) + 1;

    return copied;
}

FORCEINLINE LONG PhpCompareRightNatural(
    __in PWSTR A,
    __in PWSTR B
    )
{
    LONG bias = 0;

    for (; ; A++, B++)
    {
        if (!PhIsDigitCharacter(*A) && !PhIsDigitCharacter(*B))
        {
            return bias;
        }
        else if (!PhIsDigitCharacter(*A))
        {
            return -1;
        }
        else if (!PhIsDigitCharacter(*B))
        {
            return 1;
        }
        else if (*A < *B)
        {
            if (bias == 0)
                bias = -1;
        }
        else if (*A > *B)
        {
            if (bias == 0)
                bias = 1;
        }
        else if (!*A && !*B)
        {
            return bias;
        }
    }

    return 0;
}

FORCEINLINE LONG PhpCompareLeftNatural(
    __in PWSTR A,
    __in PWSTR B
    )
{
    for (; ; A++, B++)
    {
        if (!PhIsDigitCharacter(*A) && !PhIsDigitCharacter(*B))
        {
            return 0;
        }
        else if (!PhIsDigitCharacter(*A))
        {
            return -1;
        }
        else if (!PhIsDigitCharacter(*B))
        {
            return 1;
        }
        else if (*A < *B)
        {
            return -1;
        }
        else if (*A > *B)
        {
            return 1;
        }
    }

    return 0;
}

FORCEINLINE LONG PhpCompareUnicodeStringZNatural(
    __in PWSTR A,
    __in PWSTR B,
    __in BOOLEAN IgnoreCase
    )
{
    /*  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
        Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>

        This software is provided 'as-is', without any express or implied
        warranty.  In no event will the authors be held liable for any damages
        arising from the use of this software.

        Permission is granted to anyone to use this software for any purpose,
        including commercial applications, and to alter it and redistribute it
        freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
         claim that you wrote the original software. If you use this software
         in a product, an acknowledgment in the product documentation would be
         appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not be
         misrepresented as being the original software.
        3. This notice may not be removed or altered from any source distribution.

        This code has been modified for Process Hacker.
     */

    ULONG ai, bi;
    WCHAR ca, cb;
    LONG result;
    BOOLEAN fractional;

    ai = 0;
    bi = 0;

    while (TRUE)
    {
        ca = A[ai];
        cb = B[bi];

        /* Skip over leading spaces or zeros. */

        while (ca == ' ')
            ca = A[++ai];

        while (cb == ' ')
            cb = B[++bi];

        /* Process run of digits. */
        if (PhIsDigitCharacter(ca) && PhIsDigitCharacter(cb))
        {
            fractional = (ca == '0' || cb == '0');

            if (fractional)
            {
                if ((result = PhpCompareLeftNatural(A + ai, B + bi)) != 0)
                    return result;
            }
            else
            {
                if ((result = PhpCompareRightNatural(A + ai, B + bi)) != 0)
                    return result;
            }
        }

        if (!ca && !cb)
        {
            /* Strings are considered the same. */
            return 0;
        }

        if (IgnoreCase)
        {
            ca = towupper(ca);
            cb = towupper(cb);
        }

        if (ca < cb)
            return -1;
        else if (ca > cb)
            return 1;

        ai++;
        bi++;
    }
}

/**
 * Compares two strings in natural sort order.
 *
 * \param A The first string.
 * \param B The second string.
 * \param IgnoreCase Whether to ignore character cases.
 */
LONG PhCompareUnicodeStringZNatural(
    __in PWSTR A,
    __in PWSTR B,
    __in BOOLEAN IgnoreCase
    )
{
    if (!IgnoreCase)
        return PhpCompareUnicodeStringZNatural(A, B, FALSE);
    else
        return PhpCompareUnicodeStringZNatural(A, B, TRUE);
}

/**
 * Locates a string in a string.
 *
 * \param String1 The string to search.
 * \param String2 The string to search for.
 * \param IgnoreCase TRUE to perform a case-insensitive search, otherwise
 * FALSE.
 *
 * \return The index, in characters, of the first occurrence of
 * \a String2 in \a String1 after \a StartIndex. If \a String2 was not
 * found, -1 is returned.
 */
ULONG PhFindStringInStringRef(
    __in PPH_STRINGREF String1,
    __in PPH_STRINGREF String2,
    __in BOOLEAN IgnoreCase
    )
{
    PWSTR string1;
    ULONG length1;
    PWSTR string2;
    ULONG length2;
    ULONG i;

    string1 = String1->Buffer;
    length1 = String1->Length / sizeof(WCHAR);
    string2 = String2->Buffer;
    length2 = String2->Length / sizeof(WCHAR);

    // Can't be a substring if it's bigger than the first string.
    if (length2 > length1)
        return -1;

    if (!IgnoreCase)
    {
        for (i = length1 - length2 + 1; i != 0; i--)
        {
            PWSTR s1;
            PWSTR s2;
            ULONG l2;

            s1 = string1;
            s2 = string2;
            l2 = length2 & -2;

            if (l2)
            {
                do
                {
                    if (*(PULONG)s1 != *(PULONG)s2)
                        goto ContinueLoop;

                    s1 += 2;
                    s2 += 2;
                    l2 -= 2;
                } while (l2);
            }

            if (length2 & 1)
            {
                if (*s1 == *s2)
                    goto FoundString;
            }
            else
            {
                goto FoundString;
            }

ContinueLoop:
            string1++;
        }

        return -1;
FoundString:
        return (ULONG)(string1 - String1->Buffer);
    }
    else
    {
        UNICODE_STRING us1;
        UNICODE_STRING us2;

        us1 = String1->us;
        us2 = String2->us;
        us1.Length = us2.Length;

        for (i = length1 - length2 + 1; i != 0; i--)
        {
            if (RtlEqualUnicodeString(&us1, &us2, TRUE))
                goto FoundUString;

            us1.Buffer++;
        }

        return -1;
FoundUString:
        return (ULONG)(us1.Buffer - String1->Buffer);
    }
}

/**
 * Splits a string.
 *
 * \param Input The input string.
 * \param Separator The character to split at.
 * \param FirstPart A variable which receives the part of \a Input
 * before the separator. This may be the same variable as \a Input. If
 * the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which recieves the part of \a Input
 * after the separator. This may be the same variable as \a Input. If
 * the separator is not found in \a Input, this variable is set to
 * an empty string.
 *
 * \return TRUE if \a Separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefAtChar(
    __in PPH_STRINGREF Input,
    __in WCHAR Separator,
    __out PPH_STRINGREF FirstPart,
    __out PPH_STRINGREF SecondPart
    )
{
    PH_STRINGREF input;
    ULONG index;

    input = *Input; // get a copy of the input because FirstPart/SecondPart may alias Input
    index = PhFindCharInStringRef(Input, 0, Separator);

    if (index == -1)
    {
        // The separator was not found.

        FirstPart->Buffer = Input->Buffer;
        FirstPart->Length = Input->Length;
        SecondPart->Buffer = NULL;
        SecondPart->Length = 0;

        return FALSE;
    }

    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = (USHORT)(index * sizeof(WCHAR));
    SecondPart->Buffer = &input.Buffer[index + 1];
    SecondPart->Length = input.Length - (USHORT)(index * sizeof(WCHAR)) - sizeof(WCHAR);

    return TRUE;
}

/**
 * Splits a string.
 *
 * \param Input The input string.
 * \param SeparatorCharSet The characters to search for.
 * \param Flags A combination of the following:
 * \li \c RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END
 * \li \c RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET
 * \li \c RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE
 * \param FirstPart A variable which receives the part of \a Input
 * before the separator. This may be the same variable as \a Input. If
 * the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which recieves the part of \a Input
 * after the separator. This may be the same variable as \a Input. If
 * the separator is not found in \a Input, this variable is set to
 * an empty string.
 *
 * \return TRUE if \a Separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefAtCharEx(
    __in PPH_STRINGREF Input,
    __in PPH_STRINGREF SeparatorCharSet,
    __in ULONG Flags,
    __out PPH_STRINGREF FirstPart,
    __out PPH_STRINGREF SecondPart
    )
{
    PH_STRINGREF input;
    PH_STRINGREF separatorCharSet;
    USHORT prefixLength;

    input = *Input;
    separatorCharSet = *SeparatorCharSet;

    // RtlFindCharInUnicodeString expects a "valid" string.
    input.us.MaximumLength = input.us.Length;
    separatorCharSet.us.MaximumLength = separatorCharSet.us.Length;

    if (!NT_SUCCESS(RtlFindCharInUnicodeString(Flags, &input.us, &separatorCharSet.us, &prefixLength)))
    {
        // The separator was not found or an error occurred.

        FirstPart->Buffer = Input->Buffer;
        FirstPart->Length = Input->Length;
        SecondPart->Buffer = NULL;
        SecondPart->Length = 0;

        return FALSE;
    }

    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = prefixLength;
    SecondPart->Buffer = &input.Buffer[prefixLength / sizeof(WCHAR) + 1];
    SecondPart->Length = input.Length - prefixLength - sizeof(WCHAR);

    return TRUE;
}

/**
 * Splits a string.
 *
 * \param Input The input string.
 * \param Separator The string to split at.
 * \param IgnoreCase TRUE to perform a case-insensitive search, otherwise
 * FALSE.
 * \param FirstPart A variable which receives the part of \a Input
 * before the separator. This may be the same variable as \a Input. If
 * the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which recieves the part of \a Input
 * after the separator. This may be the same variable as \a Input. If
 * the separator is not found in \a Input, this variable is set to
 * an empty string.
 *
 * \return TRUE if \a Separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefAtString(
    __in PPH_STRINGREF Input,
    __in PPH_STRINGREF Separator,
    __in BOOLEAN IgnoreCase,
    __out PPH_STRINGREF FirstPart,
    __out PPH_STRINGREF SecondPart
    )
{
    PH_STRINGREF input;
    ULONG index;

    input = *Input; // get a copy of the input because FirstPart/SecondPart may alias Input
    index = PhFindStringInStringRef(Input, Separator, IgnoreCase);

    if (index == -1)
    {
        // The separator was not found.

        FirstPart->Buffer = Input->Buffer;
        FirstPart->Length = Input->Length;
        SecondPart->Buffer = NULL;
        SecondPart->Length = 0;

        return FALSE;
    }

    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = (USHORT)(index * sizeof(WCHAR));
    SecondPart->Buffer = &input.Buffer[index + Separator->Length / 2];
    SecondPart->Length = input.Length - (USHORT)(index * sizeof(WCHAR)) - Separator->Length;

    return TRUE;
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
        PhStringType
        )))
        return NULL;

    assert(!(Length & 1));
    string->us.MaximumLength = string->us.Length = (USHORT)Length;
    string->us.Buffer = string->Buffer;
    *(PWCHAR)((PCHAR)string->Buffer + Length) = 0;

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
    __in PSTR Buffer
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
    __in PSTR Buffer,
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
 * Obtains a reference to a zero-length string.
 */
PPH_STRING PhReferenceEmptyString(
    VOID
    )
{
    PPH_STRING string;
    PPH_STRING newString;

    string = PhSharedEmptyString;

    if (!string)
    {
        newString = PhCreateStringEx(NULL, 0);

        string = _InterlockedCompareExchangePointer(
            &PhSharedEmptyString,
            newString,
            NULL
            );

        if (!string)
        {
            string = newString; // success
        }
        else
        {
            PhDereferenceObject(newString);
        }
    }

    PhReferenceObject(string);

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
    SIZE_T cachedLengths[PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE];
    PWSTR arg;
    PPH_STRING string;

    // Compute the total length, in bytes, of the strings.

    argptr = ArgPtr;

    for (i = 0; i < Count; i++)
    {
        arg = va_arg(argptr, PWSTR);
        stringLength = wcslen(arg) * sizeof(WCHAR);
        totalLength += stringLength;

        if (i < PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE)
            cachedLengths[i] = stringLength;
    }

    // Create the string.

    string = PhCreateStringEx(NULL, totalLength);
    totalLength = 0;

    // Append the strings one by one.

    argptr = ArgPtr;

    for (i = 0; i < Count; i++)
    {
        arg = va_arg(argptr, PWSTR);

        if (i < PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE)
            stringLength = cachedLengths[i];
        else
            stringLength = wcslen(arg) * sizeof(WCHAR);

        memcpy(
            (PCHAR)string->Buffer + totalLength,
            arg,
            stringLength
            );
        totalLength += stringLength;
    }

    return string;
}

/**
 * Concatenates two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 */
PPH_STRING PhConcatStrings2(
    __in PWSTR String1,
    __in PWSTR String2
    )
{
    PPH_STRING string;
    SIZE_T length1;
    SIZE_T length2;

    length1 = wcslen(String1) * sizeof(WCHAR);
    length2 = wcslen(String2) * sizeof(WCHAR);
    string = PhCreateStringEx(NULL, length1 + length2);
    memcpy(
        string->Buffer,
        String1,
        length1
        );
    memcpy(
        (PCHAR)string->Buffer + length1,
        String2,
        length2
        );

    return string;
}

/**
 * Concatenates two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 */
PPH_STRING PhConcatStringRef2(
    __in PPH_STRINGREF String1,
    __in PPH_STRINGREF String2
    )
{
    PPH_STRING string;

    assert(!(String1->Length & 1));
    assert(!(String2->Length & 1));

    string = PhCreateStringEx(NULL, String1->Length + String2->Length);
    memcpy(string->Buffer, String1->Buffer, String1->Length);
    memcpy((PCHAR)string->Buffer + String1->Length, String2->Buffer, String2->Length);

    return string;
}

/**
 * Concatenates three strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param String3 The third string.
 */
PPH_STRING PhConcatStringRef3(
    __in PPH_STRINGREF String1,
    __in PPH_STRINGREF String2,
    __in PPH_STRINGREF String3
    )
{
    PPH_STRING string;
    PCHAR buffer;

    assert(!(String1->Length & 1));
    assert(!(String2->Length & 1));
    assert(!(String3->Length & 1));

    string = PhCreateStringEx(NULL, String1->Length + String2->Length + String3->Length);

    buffer = (PCHAR)string->Buffer;
    memcpy(buffer, String1->Buffer, String1->Length);

    buffer += String1->Length;
    memcpy(buffer, String2->Buffer, String2->Length);

    buffer += String2->Length;
    memcpy(buffer, String3->Buffer, String3->Length);

    return string;
}

/**
 * Creates a string using format specifiers.
 *
 * \param Format The format-control string.
 */
PPH_STRING PhFormatString(
    __in __format_string PWSTR Format,
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
    __in __format_string PWSTR Format,
    __in va_list ArgPtr
    )
{
    PPH_STRING string;
    int length;

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
        PhAnsiStringType
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
 * Creates a string object from an existing
 * null-terminated string.
 *
 * \param Buffer A null-terminated Unicode string.
 */
PPH_FULL_STRING PhCreateFullString(
    __in PWSTR Buffer
    )
{
    SIZE_T length;

    length = wcslen(Buffer) * sizeof(WCHAR);

    return PhCreateFullStringEx(Buffer, length, length);
}

/**
 * Creates a string object.
 *
 * \param InitialCapacity The number of bytes to allocate
 * for the string. This should not include space for a null
 * terminator.
 */
PPH_FULL_STRING PhCreateFullString2(
    __in SIZE_T InitialCapacity
    )
{
    return PhCreateFullStringEx(NULL, 0, InitialCapacity);
}

/**
 * Creates a string object using a specified length.
 *
 * \param Buffer A null-terminated Unicode string.
 * \param Length The length, in bytes, of the string.
 * \param InitialCapacity The number of bytes to allocate
 * for the string. This should not include space for a null
 * terminator. If the specified value is less than \a Length,
 * \a Length bytes are still allocated.
 */
PPH_FULL_STRING PhCreateFullStringEx(
    __in_opt PWSTR Buffer,
    __in SIZE_T Length,
    __in_opt SIZE_T InitialCapacity
    )
{
    PPH_FULL_STRING string;

    if (!NT_SUCCESS(PhCreateObject(
        &string,
        sizeof(PH_FULL_STRING), // null terminator
        0,
        PhFullStringType
        )))
        return NULL;

    if (InitialCapacity < Length)
        InitialCapacity = Length;

    string->Length = Length;
    string->AllocatedLength = InitialCapacity;
    string->Buffer = PhAllocate(string->AllocatedLength + sizeof(WCHAR));
    *(PWCHAR)((PCHAR)string->Buffer + Length) = 0;

    if (Buffer)
    {
        memcpy(string->Buffer, Buffer, Length);
    }

    return string;
}

VOID NTAPI PhpFullStringDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_FULL_STRING string = (PPH_FULL_STRING)Object;

    PhFree(string->Buffer);
}

FORCEINLINE VOID PhpWriteNullTerminatorFullString(
    __in PPH_FULL_STRING String
    )
{
    assert(!(String->Length & 1));
    *(PWCHAR)((PCHAR)String->Buffer + String->Length) = 0;
}

/**
 * Resizes a string object.
 *
 * \param String A string object.
 * \param NewLength The new required length of the string object.
 * This should not include space for a null terminator. If
 * \a Growing is TRUE, \a NewLength must not be smaller than the
 * current allocated length.
 * \param Growing TRUE to use sizing logic for growing strings,
 * otherwise FALSE to resize to the exact specified length.
 */
VOID PhResizeFullString(
    __inout PPH_FULL_STRING String,
    __in SIZE_T NewLength,
    __in BOOLEAN Growing
    )
{
    if (Growing)
    {
        assert(NewLength >= String->AllocatedLength);

        // Double the string size. If that still isn't
        // enough room, just use the new length.
        String->AllocatedLength *= 2;

        if (String->AllocatedLength < NewLength)
            String->AllocatedLength = NewLength;
    }
    else
    {
        String->AllocatedLength = NewLength;

        // This check only applies when we're shortening the string.
        if (String->Length > String->AllocatedLength)
            String->Length = String->AllocatedLength;
    }

    // Resize the buffer.
    String->Buffer = PhReAllocate(String->Buffer, String->AllocatedLength + sizeof(WCHAR));
    // Make sure we have a null terminator.
    PhpWriteNullTerminatorFullString(String);
}

/**
 * Appends a string to the end of a string.
 *
 * \param String A string object.
 * \param ShortString The string to append.
 */
VOID PhAppendFullString(
    __inout PPH_FULL_STRING String,
    __in PPH_STRING ShortString
    )
{
    PhAppendFullStringEx(
        String,
        ShortString->Buffer,
        ShortString->Length
        );
}

/**
 * Appends a string to the end of a string.
 *
 * \param String A string object.
 * \param StringZ The string to append.
 */
VOID PhAppendFullString2(
    __inout PPH_FULL_STRING String,
    __in PWSTR StringZ
    )
{
    PhAppendFullStringEx(
        String,
        StringZ,
        wcslen(StringZ) * sizeof(WCHAR)
        );
}

/**
 * Appends a string to the end of a string.
 *
 * \param String A string object.
 * \param Buffer The string to append. Specify NULL
 * to simply reserve \a Length bytes.
 * \param Length The number of bytes to append.
 */
VOID PhAppendFullStringEx(
    __inout PPH_FULL_STRING String,
    __in_opt PWSTR Buffer,
    __in SIZE_T Length
    )
{
    if (Length == 0)
        return;

    // Resize the string if necessary.
    if (String->AllocatedLength < String->Length + Length)
        PhResizeFullString(String, String->Length + Length, TRUE);

    if (Buffer)
    {
        memcpy(
            (PCHAR)String->Buffer + String->Length,
            Buffer,
            Length
            );
    }

    String->Length += Length;
    PhpWriteNullTerminatorFullString(String);
}

/**
 * Appends a character to the end of a string.
 *
 * \param String A string object.
 * \param Character The character to append.
 */
VOID PhAppendCharFullString(
    __inout PPH_FULL_STRING String,
    __in WCHAR Character
    )
{
    if (String->AllocatedLength < String->Length + sizeof(WCHAR))
        PhResizeFullString(String, String->Length + sizeof(WCHAR), TRUE);

    *(PWCHAR)((PCHAR)String->Buffer + String->Length) = Character;
    String->Length += sizeof(WCHAR);
    PhpWriteNullTerminatorFullString(String);
}

/**
 * Appends a number of characters to the end of a string.
 *
 * \param String A string object.
 * \param Character The character to append.
 * \param Count The number of times to append the character.
 */
VOID PhAppendCharFullString2(
    __inout PPH_FULL_STRING String,
    __in WCHAR Character,
    __in SIZE_T Count
    )
{
    if (Count == 0)
        return;

    if (String->AllocatedLength < String->Length + Count * sizeof(WCHAR))
        PhResizeFullString(String, String->Length + Count * sizeof(WCHAR), TRUE);

    wmemset(
        (PWCHAR)((PCHAR)String->Buffer + String->Length),
        Character,
        Count
        );

    String->Length += Count * sizeof(WCHAR);
    PhpWriteNullTerminatorFullString(String);
}

/**
 * Appends a formatted string to the end of a string.
 *
 * \param String A string object.
 * \param Format The format-control string.
 */
VOID PhAppendFormatFullString(
    __inout PPH_FULL_STRING String,
    __in __format_string PWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);
    PhAppendFormatFullString_V(String, Format, argptr);
}

VOID PhAppendFormatFullString_V(
    __inout PPH_FULL_STRING String,
    __in __format_string PWSTR Format,
    __in va_list ArgPtr
    )
{
    int length;
    SIZE_T lengthInBytes;

    length = _vscwprintf(Format, ArgPtr);

    if (length == -1 || length == 0)
        return;

    lengthInBytes = (SIZE_T)(ULONG)length * sizeof(WCHAR);

    if (String->AllocatedLength < String->Length + lengthInBytes)
        PhResizeFullString(String, String->Length + lengthInBytes, TRUE);

    _vsnwprintf(
        (PWCHAR)((PCHAR)String->Buffer + String->Length),
        length,
        Format,
        ArgPtr
        );

    String->Length += lengthInBytes;
    PhpWriteNullTerminatorFullString(String);
}

/**
 * Inserts a string into a string.
 *
 * \param String A string object.
 * \param Index The index, in characters, at which to
 * insert the string.
 * \param ShortString The string to insert.
 */
VOID PhInsertFullString(
    __inout PPH_FULL_STRING String,
    __in SIZE_T Index,
    __in PPH_STRING ShortString
    )
{
    PhInsertFullStringEx(
        String,
        Index,
        ShortString->Buffer,
        ShortString->Length
        );
}

/**
 * Inserts a string into a string.
 *
 * \param String A string object.
 * \param Index The index, in characters, at which to
 * insert the string.
 * \param StringZ The string to insert.
 */
VOID PhInsertFullString2(
    __inout PPH_FULL_STRING String,
    __in SIZE_T Index,
    __in PWSTR StringZ
    )
{
    PhInsertFullStringEx(
        String,
        Index,
        StringZ,
        wcslen(StringZ) * sizeof(WCHAR)
        );
}

/**
 * Inserts a string into a string.
 *
 * \param String A string object.
 * \param Index The index, in characters, at which to
 * insert the string.
 * \param Buffer The string to insert. Specify NULL to
 * simply reserve \a Length bytes at \a Index.
 * \param Length The number of bytes to insert.
 */
VOID PhInsertFullStringEx(
    __inout PPH_FULL_STRING String,
    __in SIZE_T Index,
    __in_opt PWSTR Buffer,
    __in SIZE_T Length
    )
{
    if (Length == 0)
        return;

    // Resize the string if necessary.
    if (String->AllocatedLength < String->Length + Length)
        PhResizeFullString(String, String->Length + Length, TRUE);

    if (Index * sizeof(WCHAR) < String->Length)
    {
        // Create some space for the string.
        memmove(
            &String->Buffer[Index + Length / sizeof(WCHAR)],
            &String->Buffer[Index],
            String->Length - Index * sizeof(WCHAR)
            );
    }

    if (Buffer)
    {
        // Copy the new string.
        memcpy(
            &String->Buffer[Index],
            Buffer,
            Length
            );
    }

    String->Length += Length;
    PhpWriteNullTerminatorFullString(String);
}

/**
 * Removes characters from a string.
 *
 * \param String A string object.
 * \param StartIndex The index, in characters, at
 * which to begin removing characters.
 * \param Count The number of characters to remove.
 */
VOID PhRemoveFullString(
    __inout PPH_FULL_STRING String,
    __in SIZE_T StartIndex,
    __in SIZE_T Count
    )
{
    // Overwrite the removed part with the part behind it.

    memmove(
        &String->Buffer[StartIndex],
        &String->Buffer[StartIndex + Count],
        String->Length - (Count + StartIndex) * sizeof(WCHAR)
        );
    String->Length -= Count * sizeof(WCHAR);
    PhpWriteNullTerminatorFullString(String);
}

/**
 * Initializes a string builder object.
 *
 * \param StringBuilder A string builder object.
 * \param InitialCapacity The number of bytes to allocate
 * initially.
 */
VOID PhInitializeStringBuilder(
    __out PPH_STRING_BUILDER StringBuilder,
    __in ULONG InitialCapacity
    )
{
    StringBuilder->AllocatedLength = InitialCapacity;

    // Allocate a PH_STRING for the string builder.
    // We will dereference it and allocate a new one
    // when we need to resize the string.

    StringBuilder->String = PhCreateStringEx(
        NULL,
        StringBuilder->AllocatedLength
        );

    // We will keep modifying the Length field of the
    // string so:
    // 1. We know how much of the string is used, and
    // 2. The user can simply get a reference to the
    //    string and use it as-is.

    StringBuilder->String->Length = 0;

    // Write the null terminator.
    StringBuilder->String->Buffer[0] = 0;

    PHLIB_INC_STATISTIC(BaseStringBuildersCreated);
}

/**
 * Frees resources used by a string builder object.
 *
 * \param StringBuilder A string builder object.
 */
VOID PhDeleteStringBuilder(
    __inout PPH_STRING_BUILDER StringBuilder
    )
{
    PhDereferenceObject(StringBuilder->String);
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
        StringBuilder->String->Length + sizeof(WCHAR) // include null terminator
        );

    // Copy the old string length.
    newString->Length = StringBuilder->String->Length;

    // Dereference the old string and replace it with
    // the new string.
    PhDereferenceObject(StringBuilder->String);
    StringBuilder->String = newString;

    PHLIB_INC_STATISTIC(BaseStringBuildersResized);
}

FORCEINLINE VOID PhpWriteNullTerminatorStringBuilder(
    __in PPH_STRING_BUILDER StringBuilder
    )
{
    assert(!(StringBuilder->String->Length & 1));
    *(PWCHAR)((PCHAR)StringBuilder->String->Buffer + StringBuilder->String->Length) = 0;
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
 * Obtains a reference to the string constructed
 * by a string builder object and frees resources
 * used by the object.
 *
 * \param StringBuilder A string builder object.
 *
 * \return A pointer to a string. You must free
 * the string using PhDereferenceObject() when
 * you no longer need it.
 *
 * \remarks This function is equivalent to calling
 * PhReferenceStringBuilderString() followed by
 * PhDeleteStringBuilder().
 */
PPH_STRING PhFinalStringBuilderString(
    __inout PPH_STRING_BUILDER StringBuilder
    )
{
    return StringBuilder->String;
}

/**
 * Appends a string to the end of a string builder
 * string.
 *
 * \param StringBuilder A string builder object.
 * \param String The string to append.
 */
VOID PhAppendStringBuilder(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PPH_STRING String
    )
{
    PhAppendStringBuilderEx(
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
VOID PhAppendStringBuilder2(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in PWSTR String
    )
{
    PhAppendStringBuilderEx(
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
 * \param String The string to append. Specify NULL to
 * simply reserve \a Length bytes.
 * \param Length The number of bytes to append.
 */
VOID PhAppendStringBuilderEx(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in_opt PWSTR String,
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

    if (String)
    {
        memcpy(
            (PCHAR)StringBuilder->String->Buffer + StringBuilder->String->Length,
            String,
            Length
            );
    }

    StringBuilder->String->Length += (USHORT)Length;
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Appends a character to the end of a string builder
 * string.
 *
 * \param StringBuilder A string builder object.
 * \param Character The character to append.
 */
VOID PhAppendCharStringBuilder(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in WCHAR Character
    )
{
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + sizeof(WCHAR))
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + sizeof(WCHAR));
    }

    *(PWCHAR)((PCHAR)StringBuilder->String->Buffer + StringBuilder->String->Length) = Character;
    StringBuilder->String->Length += sizeof(WCHAR);
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Appends a number of characters to the end of a string
 * builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Character The character to append.
 * \param Count The number of times to append the character.
 */
VOID PhAppendCharStringBuilder2(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in WCHAR Character,
    __in ULONG Count
    )
{
    if (Count == 0)
        return;

    // See if we need to re-allocate the string.
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + Count * sizeof(WCHAR))
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + Count * sizeof(WCHAR));
    }

    wmemset(
        (PWCHAR)((PCHAR)StringBuilder->String->Buffer + StringBuilder->String->Length),
        Character,
        Count
        );

    StringBuilder->String->Length += (USHORT)(Count * sizeof(WCHAR));
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Appends a formatted string to the end of a string builder
 * string.
 *
 * \param StringBuilder A string builder object.
 * \param Format The format-control string.
 */
VOID PhAppendFormatStringBuilder(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in __format_string PWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);
    PhAppendFormatStringBuilder_V(StringBuilder, Format, argptr);
}

VOID PhAppendFormatStringBuilder_V(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in __format_string PWSTR Format,
    __in va_list ArgPtr
    )
{
    int length;
    ULONG lengthInBytes;

    length = _vscwprintf(Format, ArgPtr);

    if (length == -1 || length == 0)
        return;

    lengthInBytes = (ULONG)length * sizeof(WCHAR);

    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + lengthInBytes)
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + lengthInBytes);

    _vsnwprintf(
        (PWCHAR)((PCHAR)StringBuilder->String->Buffer + StringBuilder->String->Length),
        length,
        Format,
        ArgPtr
        );

    StringBuilder->String->Length += (USHORT)lengthInBytes;
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Inserts a string into a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Index The index, in characters, at which to
 * insert the string.
 * \param String The string to insert.
 */
VOID PhInsertStringBuilder(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG Index,
    __in PPH_STRING String
    )
{
    PhInsertStringBuilderEx(
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
VOID PhInsertStringBuilder2(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG Index,
    __in PWSTR String
    )
{
    PhInsertStringBuilderEx(
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
 * \param String The string to insert. Specify NULL to
 * simply reserve \a Length bytes at \a Index.
 * \param Length The number of bytes to insert.
 */
VOID PhInsertStringBuilderEx(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG Index,
    __in_opt PWSTR String,
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

    if (Index * sizeof(WCHAR) < StringBuilder->String->Length)
    {
        // Create some space for the string.
        memmove(
            &StringBuilder->String->Buffer[Index + Length / sizeof(WCHAR)],
            &StringBuilder->String->Buffer[Index],
            StringBuilder->String->Length - Index * sizeof(WCHAR)
            );
    }

    if (String)
    {
        // Copy the new string.
        memcpy(
            &StringBuilder->String->Buffer[Index],
            String,
            Length
            );
    }

    StringBuilder->String->Length += (USHORT)Length;
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Removes characters from a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param StartIndex The index, in characters, at
 * which to begin removing characters.
 * \param Count The number of characters to remove.
 */
VOID PhRemoveStringBuilder(
    __inout PPH_STRING_BUILDER StringBuilder,
    __in ULONG StartIndex,
    __in ULONG Count
    )
{
    // Overwrite the removed part with the part
    // behind it.

    memmove(
        &StringBuilder->String->Buffer[StartIndex],
        &StringBuilder->String->Buffer[StartIndex + Count],
        StringBuilder->String->Length - (Count + StartIndex) * sizeof(WCHAR)
        );
    StringBuilder->String->Length -= (USHORT)(Count * sizeof(WCHAR));
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
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
        PhListType
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
 * Resizes a list.
 *
 * \param List A list object.
 * \param NewCapacity The new required number of elements for
 * which storage has been reserved. This must not be smaller
 * than the current number of items in the list.
 */
VOID PhResizeList(
    __inout PPH_LIST List,
    __in ULONG NewCapacity
    )
{
    if (List->Count > NewCapacity)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);

    List->AllocatedCount = NewCapacity;
    List->Items = PhReAllocate(List->Items, List->AllocatedCount * sizeof(PVOID));
}

/**
 * Adds an item to a list.
 *
 * \param List A list object.
 * \param Item The item to add.
 */
VOID PhAddItemList(
    __inout PPH_LIST List,
    __in PVOID Item
    )
{
    // See if we need to resize the list.
    if (List->Count == List->AllocatedCount)
    {
        List->AllocatedCount *= 2;
        List->Items = PhReAllocate(List->Items, List->AllocatedCount * sizeof(PVOID));
    }

    List->Items[List->Count++] = Item;
}

/**
 * Adds items to a list.
 *
 * \param List A list object.
 * \param Items An array containing the items to add.
 * \param Count The number of items to add.
 */
VOID PhAddItemsList(
    __inout PPH_LIST List,
    __in PVOID *Items,
    __in ULONG Count
    )
{
    // See if we need to resize the list.
    if (List->AllocatedCount < List->Count + Count)
    {
        List->AllocatedCount *= 2;

        if (List->AllocatedCount < List->Count + Count)
            List->AllocatedCount = List->Count + Count;

        List->Items = PhReAllocate(List->Items, List->AllocatedCount * sizeof(PVOID));
    }

    memcpy(
        &List->Items[List->Count],
        Items,
        Count * sizeof(PVOID)
        );

    List->Count += Count;
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

__success(return != -1)
/**
 * Locates an item in a list.
 *
 * \param List A list object.
 * \param Item The item to search for.
 *
 * \return The index of the item. If the
 * item was not found, -1 is returned.
 */
ULONG PhFindItemList(
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
 * Inserts an item into a list.
 *
 * \param List A list object.
 * \param Index The index at which to insert the item.
 * \param Item The item to add.
 */
VOID PhInsertItemList(
    __inout PPH_LIST List,
    __in ULONG Index,
    __in PVOID Item
    )
{
    PhInsertItemsList(List, Index, &Item, 1);
}

/**
 * Inserts items into a list.
 *
 * \param List A list object.
 * \param Index The index at which to insert the items.
 * \param Items An array containing the items to add.
 * \param Count The number of items to add.
 */
VOID PhInsertItemsList(
    __inout PPH_LIST List,
    __in ULONG Index,
    __in PVOID *Items,
    __in ULONG Count
    )
{
    // See if we need to resize the list.
    if (List->AllocatedCount < List->Count + Count)
    {
        List->AllocatedCount *= 2;

        if (List->AllocatedCount < List->Count + Count)
            List->AllocatedCount = List->Count + Count;

        List->Items = PhReAllocate(List->Items, List->AllocatedCount * sizeof(PVOID));
    }

    if (Index < List->Count)
    {
        // Shift the existing items backward.
        memmove(
            &List->Items[Index + Count],
            &List->Items[Index],
            (List->Count - Index) * sizeof(PVOID)
            );
    }

    // Copy the new items into the list.
    memcpy(
        &List->Items[Index],
        Items,
        Count * sizeof(PVOID)
        );

    List->Count += Count;
}

/**
 * Removes an item from a list.
 *
 * \param List A list object.
 * \param Index The index of the item.
 */
VOID PhRemoveItemList(
    __inout PPH_LIST List,
    __in ULONG Index
    )
{
    PhRemoveItemsList(List, Index, 1);
}

/**
 * Removes items from a list.
 *
 * \param List A list object.
 * \param StartIndex The index at which to begin
 * removing items.
 * \param Count The number of items to remove.
 */
VOID PhRemoveItemsList(
    __inout PPH_LIST List,
    __in ULONG StartIndex,
    __in ULONG Count
    )
{
    // Shift the items after the items forward.
    memmove(
        &List->Items[StartIndex],
        &List->Items[StartIndex + Count],
        (List->Count - StartIndex - Count) * sizeof(PVOID)
        );

    List->Count -= Count;
}

typedef struct _PH_LIST_QSORT_CONTEXT
{
    PPH_COMPARE_FUNCTION CompareFunction;
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
 *
 * \remarks Do not use this function; use qsort instead.
 */
VOID PhSortList(
    __in PPH_LIST List,
    __in PPH_COMPARE_FUNCTION CompareFunction,
    __in_opt PVOID Context
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
 * Creates a pointer list object.
 *
 * \param InitialCapacity The number of elements to
 * allocate storage for initially.
 */
PPH_POINTER_LIST PhCreatePointerList(
    __in ULONG InitialCapacity
    )
{
    PPH_POINTER_LIST pointerList;

    if (!NT_SUCCESS(PhCreateObject(
        &pointerList,
        sizeof(PH_POINTER_LIST),
        0,
        PhPointerListType
        )))
        return NULL;

    // Initial capacity of 0 is not allowed.
    if (InitialCapacity == 0)
        InitialCapacity = 1;

    pointerList->Count = 0;
    pointerList->AllocatedCount = InitialCapacity;
    pointerList->FreeEntry = -1;
    pointerList->NextEntry = 0;
    pointerList->Items = PhAllocate(pointerList->AllocatedCount * sizeof(PVOID));

    return pointerList;
}

VOID NTAPI PhpPointerListDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_POINTER_LIST pointerList = (PPH_POINTER_LIST)Object;

    PhFree(pointerList->Items);
}

/**
 * Decodes an index stored in a free entry.
 */
FORCEINLINE ULONG PhpDecodePointerListIndex(
    __in PVOID Index
    )
{
    // At least with Microsoft's compiler, shift right on
    // a signed value preserves the sign. This is important
    // because we want
    // decode(encode(-1)) = ((-1 << 1) | 1) >> 1 = -1.
    return (ULONG)((LONG_PTR)Index >> 1);
}

/**
 * Encodes an index for storage in a free entry.
 */
FORCEINLINE PVOID PhpEncodePointerListIndex(
    __in ULONG Index
    )
{
    return (PVOID)(((ULONG_PTR)Index << 1) | 0x1);
}

FORCEINLINE HANDLE PhpPointerListIndexToHandle(
    __in ULONG Index
    )
{
    // Add one to allow NULL handles to indicate
    // failure/an invalid index.
    return (HANDLE)(Index + 1);
}

FORCEINLINE ULONG PhpPointerListHandleToIndex(
    __in HANDLE Handle
    )
{
    return (ULONG)Handle - 1;
}

/**
 * Adds a pointer to a pointer list.
 *
 * \param PointerList A pointer list object.
 * \param Pointer The pointer to add. The pointer
 * must be at least 2 byte aligned.
 *
 * \return A handle to the pointer, valid until
 * the pointer is removed from the pointer list.
 */
HANDLE PhAddItemPointerList(
    __inout PPH_POINTER_LIST PointerList,
    __in PVOID Pointer
    )
{
    ULONG index;

    assert(PH_IS_LIST_POINTER_VALID(Pointer));

    // Use a free entry if possible.
    if (PointerList->FreeEntry != -1)
    {
        PVOID oldPointer;

        index = PointerList->FreeEntry;
        oldPointer = PointerList->Items[index];
        PointerList->Items[index] = Pointer;
        PointerList->FreeEntry = PhpDecodePointerListIndex(oldPointer);
    }
    else
    {
        // Use the next entry.
        if (PointerList->NextEntry == PointerList->AllocatedCount)
        {
            PointerList->AllocatedCount *= 2;
            PointerList->Items = PhReAllocate(PointerList->Items, PointerList->AllocatedCount * sizeof(PVOID));
        }

        index = PointerList->NextEntry++;
        PointerList->Items[index] = Pointer;
    }

    PointerList->Count++;

    return PhpPointerListIndexToHandle(index);
}

BOOLEAN PhEnumPointerListEx(
    __in PPH_POINTER_LIST PointerList,
    __inout PULONG EnumerationKey,
    __out PVOID *Pointer,
    __out PHANDLE PointerHandle
    )
{
    ULONG index;

    while ((index = *EnumerationKey) < PointerList->NextEntry)
    {
        PVOID pointer = PointerList->Items[index];

        (*EnumerationKey)++;

        if (PH_IS_LIST_POINTER_VALID(pointer))
        {
            *Pointer = pointer;
            *PointerHandle = PhpPointerListIndexToHandle(index);

            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Locates a pointer in a pointer list.
 *
 * \param PointerList A pointer list object.
 * \param Pointer The pointer to find. The pointer
 * must be at least 2 byte aligned.
 *
 * \return A handle to the pointer, valid until
 * the pointer is removed from the pointer list.
 * If the pointer is not contained in the pointer
 * list, NULL is returned.
 */
HANDLE PhFindItemPointerList(
    __in PPH_POINTER_LIST PointerList,
    __in PVOID Pointer
    )
{
    ULONG i;

    assert(PH_IS_LIST_POINTER_VALID(Pointer));

    for (i = 0; i < PointerList->NextEntry; i++)
    {
        if (PointerList->Items[i] == Pointer)
            return PhpPointerListIndexToHandle(i);
    }

    return NULL;
}

/**
 * Removes a pointer from a pointer list.
 *
 * \param PointerList A pointer list object.
 * \param PointerHandle A handle to the pointer
 * to remove.
 *
 * \remarks No checking is performed on the
 * pointer handle. Make sure the handle is valid
 * before calling the function.
 */
VOID PhRemoveItemPointerList(
    __inout PPH_POINTER_LIST PointerList,
    __in HANDLE PointerHandle
    )
{
    ULONG index;

    assert(PointerHandle);

    index = PhpPointerListHandleToIndex(PointerHandle);

    PointerList->Items[index] = PhpEncodePointerListIndex(PointerList->FreeEntry);
    PointerList->FreeEntry = index;

    PointerList->Count--;
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
        PhQueueType
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
VOID PhEnqueueItemQueue(
    __inout PPH_QUEUE Queue,
    __in PVOID Item
    )
{
    // See if we need to resize the queue.
    if (Queue->Count == Queue->AllocatedCount)
    {
        ULONG oldAllocatedCount = Queue->AllocatedCount;
        PVOID *oldItems = Queue->Items;

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
BOOLEAN PhDequeueItemQueue(
    __inout PPH_QUEUE Queue,
    __out PVOID *Item
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
BOOLEAN PhPeekItemQueue(
    __in PPH_QUEUE Queue,
    __out PVOID *Item
    )
{
    if (Queue->Count == 0)
        return FALSE;

    *Item = Queue->Items[Queue->Head];

    return TRUE;
}

FORCEINLINE ULONG PhpValidateHash(
    __in ULONG Hash
    )
{
    // No point in using a full hash when we're going to
    // AND with size minus one anyway.
#if defined(PH_HASHTABLE_FULL_HASH) && !defined(PH_HASHTABLE_POWER_OF_TWO_SIZE)
    if (Hash != -1)
        return Hash;
    else
        return 0;
#else
    return Hash & MAXLONG;
#endif
}

FORCEINLINE ULONG PhpIndexFromHash(
    __in PPH_HASHTABLE Hashtable,
    __in ULONG Hash
    )
{
#ifdef PH_HASHTABLE_POWER_OF_TWO_SIZE
    return Hash & (Hashtable->AllocatedBuckets - 1);
#else
    return Hash % Hashtable->AllocatedBuckets;
#endif
}

FORCEINLINE ULONG PhpGetNumberOfBuckets(
    __in ULONG Capacity
    )
{
#ifdef PH_HASHTABLE_POWER_OF_TWO_SIZE
    return PhRoundUpToPowerOfTwo(Capacity);
#else
    return PhGetPrimeNumber(Capacity);
#endif
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
        PhHashtableType
        )))
        return NULL;

    // Initial capacity of 0 is not allowed.
    if (InitialCapacity == 0)
        InitialCapacity = 1;

    hashtable->EntrySize = EntrySize;
    hashtable->CompareFunction = CompareFunction;
    hashtable->HashFunction = HashFunction;

    // Allocate the buckets.
    hashtable->AllocatedBuckets = PhpGetNumberOfBuckets(InitialCapacity);
    hashtable->Buckets = PhAllocate(sizeof(ULONG) * hashtable->AllocatedBuckets);
    // Set all bucket values to -1.
    memset(hashtable->Buckets, 0xff, sizeof(ULONG) * hashtable->AllocatedBuckets);

    // Allocate the entries.
    hashtable->AllocatedEntries = hashtable->AllocatedBuckets;
    hashtable->Entries = PhAllocate(PH_HASHTABLE_ENTRY_SIZE(EntrySize) * hashtable->AllocatedEntries);

    hashtable->Count = 0;
    hashtable->FreeEntry = -1;
    hashtable->NextEntry = 0;

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

VOID PhpResizeHashtable(
    __inout PPH_HASHTABLE Hashtable,
    __in ULONG NewCapacity
    )
{
    PPH_HASHTABLE_ENTRY entry;
    ULONG i;

    // Re-allocate the buckets. Note that we don't need to keep the
    // contents.
    Hashtable->AllocatedBuckets = PhpGetNumberOfBuckets(NewCapacity);
    PhFree(Hashtable->Buckets);
    Hashtable->Buckets = PhAllocate(sizeof(ULONG) * Hashtable->AllocatedBuckets);
    // Set all bucket values to -1.
    memset(Hashtable->Buckets, 0xff, sizeof(ULONG) * Hashtable->AllocatedBuckets);

    // Re-allocate the entries.
    Hashtable->AllocatedEntries = Hashtable->AllocatedBuckets;
    Hashtable->Entries = PhReAllocate(
        Hashtable->Entries,
        PH_HASHTABLE_ENTRY_SIZE(Hashtable->EntrySize) * Hashtable->AllocatedEntries
        );

    // Re-distribute the entries among the buckets.

    // PH_HASHTABLE_GET_ENTRY is quite slow (it involves a multiply), so we use a pointer here.
    entry = Hashtable->Entries;

    for (i = 0; i < Hashtable->NextEntry; i++)
    {
        if (entry->HashCode != -1)
        {
            ULONG index = PhpIndexFromHash(Hashtable, entry->HashCode);

            entry->Next = Hashtable->Buckets[index];
            Hashtable->Buckets[index] = i;
        }

        entry = (PPH_HASHTABLE_ENTRY)((ULONG_PTR)entry + PH_HASHTABLE_ENTRY_SIZE(Hashtable->EntrySize));
    }
}

FORCEINLINE PVOID PhpAddEntryHashtable(
    __inout PPH_HASHTABLE Hashtable,
    __in PVOID Entry,
    __in BOOLEAN CheckForDuplicate,
    __out_opt PBOOLEAN Added
    )
{
    ULONG hashCode; // hash code of the new entry
    ULONG index; // bucket index of the new entry
    ULONG freeEntry; // index of new entry in entry array
    PPH_HASHTABLE_ENTRY entry; // pointer to new entry in entry array

    hashCode = PhpValidateHash(Hashtable->HashFunction(Entry));
    index = PhpIndexFromHash(Hashtable, hashCode);

    if (CheckForDuplicate)
    {
        ULONG i;

        for (i = Hashtable->Buckets[index]; i != -1; i = entry->Next)
        {
            entry = PH_HASHTABLE_GET_ENTRY(Hashtable, i);

            if (entry->HashCode == hashCode && Hashtable->CompareFunction(&entry->Body, Entry))
            {
                if (Added)
                    *Added = FALSE;

                return &entry->Body;
            }
        }
    }

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
            index = PhpIndexFromHash(Hashtable, hashCode);
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

    if (Added)
        *Added = TRUE;

    return &entry->Body;
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
 *
 * \remarks Entries are only guaranteed to be 8 byte
 * aligned, even on 64-bit systems.
 */
PVOID PhAddEntryHashtable(
    __inout PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    )
{
    PVOID entry;
    BOOLEAN added;

    entry = PhpAddEntryHashtable(Hashtable, Entry, TRUE, &added);

    if (added)
        return entry;
    else
        return NULL;
}

/**
 * Adds an entry to a hashtable or returns an existing one.
 *
 * \param Hashtable A hashtable object.
 * \param Entry The entry to add.
 * \param Added A variable which receives TRUE if a new
 * entry was created, and FALSE if an existing entry was
 * returned.
 *
 * \return A pointer to the entry as stored in
 * the hashtable. This pointer is valid until
 * the hashtable is modified. If the hashtable
 * already contained an equal entry, the existing entry
 * is returned. Check the value of \a Added to determine
 * whether the returned entry is new or existing.
 *
 * \remarks Entries are only guaranteed to be 8 byte
 * aligned, even on 64-bit systems.
 */
PVOID PhAddEntryHashtableEx(
    __inout PPH_HASHTABLE Hashtable,
    __in PVOID Entry,
    __out_opt PBOOLEAN Added
    )
{
    return PhpAddEntryHashtable(Hashtable, Entry, TRUE, Added);
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
    __out PVOID *Entry,
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
PVOID PhFindEntryHashtable(
    __in PPH_HASHTABLE Hashtable,
    __in PVOID Entry
    )
{
    ULONG hashCode;
    ULONG index;
    ULONG i;
    PPH_HASHTABLE_ENTRY entry;

    hashCode = PhpValidateHash(Hashtable->HashFunction(Entry));
    index = PhpIndexFromHash(Hashtable, hashCode);

    for (i = Hashtable->Buckets[index]; i != -1; i = entry->Next)
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
 * by PhFindEntryHashtable, or a partial
 * entry.
 */
BOOLEAN PhRemoveEntryHashtable(
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
    index = PhpIndexFromHash(Hashtable, hashCode);
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
ULONG FASTCALL PhfHashBytesHsieh(
    __in PUCHAR Bytes,
    __in ULONG Length
    )
{
    // Hsieh hash, http://www.azillionmonkeys.com/qed/hash.html
    ULONG hash;
    ULONG tmp;
    ULONG rem;

    if (!Length)
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
ULONG FASTCALL PhfHashBytesMurmur(
    __in PUCHAR Bytes,
    __in ULONG Length
    )
{
    // Murmur hash, http://murmurhash.googlepages.com
#define MURMUR_MAGIC 0x5bd1e995
#define MURMUR_SHIFT 24

    ULONG hash = Length;
    ULONG tmp;

    while (Length >= 4)
    {
        tmp = *(PULONG)Bytes;

        tmp *= MURMUR_MAGIC;
        tmp ^= tmp >> MURMUR_SHIFT;
        tmp *= MURMUR_MAGIC;

        hash *= MURMUR_MAGIC;
        hash ^= tmp;

        Bytes += 4;
        Length -= 4;
    }

    switch (Length)
    {
    case 3:
        hash ^= Bytes[2] << 16;
    case 2:
        hash ^= Bytes[1] << 8;
    case 1:
        hash ^= Bytes[0];
        hash *= MURMUR_MAGIC;
    }

    hash ^= hash >> 13;
    hash *= MURMUR_MAGIC;
    hash ^= hash >> 15;

    return hash;
}

/**
 * Generates a hash code for a sequence of bytes.
 *
 * \param Bytes A pointer to a byte array.
 * \param Length The number of bytes to hash.
 */
ULONG FASTCALL PhfHashBytesSdbm(
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

BOOLEAN NTAPI PhpSimpleHashtableCompareFunction(
    __in PVOID Entry1,
    __in PVOID Entry2
    )
{
    PPH_KEY_VALUE_PAIR entry1 = Entry1;
    PPH_KEY_VALUE_PAIR entry2 = Entry2;

    return entry1->Key == entry2->Key;
}

ULONG NTAPI PhpSimpleHashtableHashFunction(
    __in PVOID Entry
    )
{
    PPH_KEY_VALUE_PAIR entry = Entry;

#ifdef _M_IX86
    return PhHashInt32((ULONG)entry->Key);
#else
    return PhHashInt64((ULONG64)entry->Key);
#endif
}

PPH_HASHTABLE PhCreateSimpleHashtable(
    __in ULONG InitialCapacity
    )
{
    return PhCreateHashtable(
        sizeof(PH_KEY_VALUE_PAIR),
        PhpSimpleHashtableCompareFunction,
        PhpSimpleHashtableHashFunction,
        InitialCapacity
        );
}

PVOID PhAddItemSimpleHashtable(
    __inout PPH_HASHTABLE SimpleHashtable,
    __in_opt PVOID Key,
    __in_opt PVOID Value
    )
{
    PH_KEY_VALUE_PAIR entry;

    entry.Key = Key;
    entry.Value = Value;

    if (PhAddEntryHashtable(SimpleHashtable, &entry))
        return Value;
    else
        return NULL;
}

PVOID *PhFindItemSimpleHashtable(
    __in PPH_HASHTABLE SimpleHashtable,
    __in_opt PVOID Key
    )
{
    PH_KEY_VALUE_PAIR lookupEntry;
    PPH_KEY_VALUE_PAIR entry;

    lookupEntry.Key = Key;
    entry = PhFindEntryHashtable(SimpleHashtable, &lookupEntry);

    if (entry)
        return &entry->Value;
    else
        return NULL;
}

BOOLEAN PhRemoveItemSimpleHashtable(
    __inout PPH_HASHTABLE SimpleHashtable,
    __in_opt PVOID Key
    )
{
    PH_KEY_VALUE_PAIR lookupEntry;

    lookupEntry.Key = Key;

    return PhRemoveEntryHashtable(SimpleHashtable, &lookupEntry);
}

/**
 * Initializes a free list object.
 *
 * \param FreeList A pointer to the free list object.
 * \param Size The number of bytes in each allocation.
 * \param MaximumCount The number of unused allocations
 * to store.
 */
VOID PhInitializeFreeList(
    __out PPH_FREE_LIST FreeList,
    __in SIZE_T Size,
    __in ULONG MaximumCount
    )
{
    RtlInitializeSListHead(&FreeList->ListHead);
    FreeList->Count = 0;
    FreeList->MaximumCount = MaximumCount;
    FreeList->Size = Size;
}

/**
 * Frees resources used by a free list object.
 *
 * \param FreeList A pointer to the free list object.
 */
VOID PhDeleteFreeList(
    __inout PPH_FREE_LIST FreeList
    )
{
    PPH_FREE_LIST_ENTRY entry;
    PSLIST_ENTRY listEntry;

    listEntry = RtlInterlockedFlushSList(&FreeList->ListHead);

    while (listEntry)
    {
        entry = CONTAINING_RECORD(listEntry, PH_FREE_LIST_ENTRY, ListEntry);
        listEntry = listEntry->Next;
        PhFree(entry);
    }
}

/**
 * Allocates a block of memory from a free list.
 *
 * \param FreeList A pointer to a free list object.
 *
 * \return A pointer to the allocated block of
 * memory. The memory must be freed using
 * PhFreeToFreeList(). The block is guaranteed to be
 * aligned at MEMORY_ALLOCATION_ALIGNMENT bytes.
 */
PVOID PhAllocateFromFreeList(
    __inout PPH_FREE_LIST FreeList
    )
{
    PPH_FREE_LIST_ENTRY entry;
    PSLIST_ENTRY listEntry;

    listEntry = RtlInterlockedPopEntrySList(&FreeList->ListHead);

    if (listEntry)
    {
        _InterlockedDecrement((PLONG)&FreeList->Count);
        entry = CONTAINING_RECORD(listEntry, PH_FREE_LIST_ENTRY, ListEntry);
    }
    else
    {
        entry = PhAllocate(FIELD_OFFSET(PH_FREE_LIST_ENTRY, Body) + FreeList->Size);
    }

    return &entry->Body;
}

/**
 * Frees a block of memory to a free list.
 *
 * \param FreeList A pointer to a free list object.
 * \param Memory A pointer to a block of memory.
 */
VOID PhFreeToFreeList(
    __inout PPH_FREE_LIST FreeList,
    __in PVOID Memory
    )
{
    PPH_FREE_LIST_ENTRY entry;

    entry = CONTAINING_RECORD(Memory, PH_FREE_LIST_ENTRY, Body);

    // We don't enforce Count <= MaximumCount (that would require locking),
    // but we do check it.
    if (FreeList->Count < FreeList->MaximumCount)
    {
        RtlInterlockedPushEntrySList(&FreeList->ListHead, &entry->ListEntry);
        _InterlockedIncrement((PLONG)&FreeList->Count);
    }
    else
    {
        PhFree(entry);
    }
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
    PhInitializeQueuedLock(&Callback->ListLock);
    PhInitializeQueuedLock(&Callback->BusyCondition);
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
    // Nothing for now
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
    __in_opt PVOID Context,
    __out PPH_CALLBACK_REGISTRATION Registration
    )
{
    PhRegisterCallbackEx(
        Callback,
        Function,
        Context,
        0,
        Registration
        );
}

/**
 * Registers a callback function to be notified.
 *
 * \param Callback A pointer to a callback object.
 * \param Function The callback function.
 * \param Context A user-defined value to pass to the
 * callback function.
 * \param Flags A combination of flags controlling the
 * callback. Set this parameter to 0.
 * \param Registration A variable which receives
 * registration information for the callback. Do not
 * modify the contents of this structure and do not
 * free the storage for this structure until you have
 * unregistered the callback.
 */
VOID PhRegisterCallbackEx(
    __inout PPH_CALLBACK Callback,
    __in PPH_CALLBACK_FUNCTION Function,
    __in_opt PVOID Context,
    __in USHORT Flags,
    __out PPH_CALLBACK_REGISTRATION Registration
    )
{
    Registration->Function = Function;
    Registration->Context = Context;
    Registration->Busy = 0;
    Registration->Unregistering = FALSE;
    Registration->Flags = Flags;

    PhAcquireQueuedLockExclusive(&Callback->ListLock);
    InsertTailList(&Callback->ListHead, &Registration->ListEntry);
    PhReleaseQueuedLockExclusive(&Callback->ListLock);
}

/**
 * Unregisters a callback function.
 *
 * \param Callback A pointer to a callback object.
 * \param Registration The structure returned by
 * PhRegisterCallback().
 *
 * \remarks It is guaranteed that the callback
 * function will not be in execution once this function
 * returns. Attempting to unregister a callback function
 * from within the same function will result in a deadlock.
 */
VOID PhUnregisterCallback(
    __inout PPH_CALLBACK Callback,
    __inout PPH_CALLBACK_REGISTRATION Registration
    )
{
    Registration->Unregistering = TRUE;

    PhAcquireQueuedLockExclusive(&Callback->ListLock);

    // Wait for the callback to be unbusy.
    while (Registration->Busy)
        PhWaitForCondition(&Callback->BusyCondition, &Callback->ListLock, NULL);

    RemoveEntryList(&Registration->ListEntry);

    PhReleaseQueuedLockExclusive(&Callback->ListLock);
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
    __in_opt PVOID Parameter
    )
{
    PLIST_ENTRY listEntry;

    PhAcquireQueuedLockShared(&Callback->ListLock);

    listEntry = Callback->ListHead.Flink;

    while (listEntry != &Callback->ListHead)
    {
        PPH_CALLBACK_REGISTRATION registration;
        LONG busy;

        registration = CONTAINING_RECORD(listEntry, PH_CALLBACK_REGISTRATION, ListEntry);

        // Don't bother executing the callback function if
        // it is being unregistered.
        if (registration->Unregistering)
            continue;

        _InterlockedIncrement(&registration->Busy);

        // Execute the callback function.

        PhReleaseQueuedLockShared(&Callback->ListLock);
        registration->Function(
            Parameter,
            registration->Context
            );
        PhAcquireQueuedLockShared(&Callback->ListLock);

        busy = _InterlockedDecrement(&registration->Busy);

        if (registration->Unregistering && busy == 0)
        {
            // Someone started unregistering while the callback
            // function was executing, and we must wake them.
            PhPulseAllCondition(&Callback->BusyCondition);
        }

        listEntry = listEntry->Flink;
    }

    PhReleaseQueuedLockShared(&Callback->ListLock);
}

/**
 * Retrieves a prime number bigger than or equal to the
 * specified number.
 */
ULONG PhGetPrimeNumber(
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
 * Rounds up a number to the next power of two.
 */
ULONG PhRoundUpToPowerOfTwo(
    __in ULONG Number
    )
{
    Number--;
    Number |= Number >> 1;
    Number |= Number >> 2;
    Number |= Number >> 4;
    Number |= Number >> 8;
    Number |= Number >> 16;
    Number++;

    return Number;
}

/**
 * Performs exponentiation.
 */
ULONG PhExponentiate(
    __in ULONG Base,
    __in ULONG Exponent
    )
{
    ULONG result = 1;

    while (Exponent)
    {
        if (Exponent & 1)
            result *= Base;

        Exponent >>= 1;
        Base *= Base;
    }

    return result;
}

/**
 * Performs 64-bit exponentiation.
 */
ULONG64 PhExponentiate64(
    __in ULONG64 Base,
    __in ULONG Exponent
    )
{
    ULONG64 result = 1;

    while (Exponent)
    {
        if (Exponent & 1)
            result *= Base;

        Exponent >>= 1;
        Base *= Base;
    }

    return result;
}

/*
 * Calculates the binary logarithm of a number.
 *
 * \return The floor of the binary logarithm of the number.
 * This is the same as the position of the highest set bit.
 */
ULONG PhLog2(
    __in ULONG Exponent
    )
{
    ULONG result = 0;

    while (Exponent)
    {
        result++;
        Exponent >>= 1;
    }

    return result;
}

/**
 * Converts a sequence of hexadecimal digits into a byte array.
 *
 * \param String A string containing hexadecimal digits to convert.
 * The string must have an even number of digits, because each pair
 * of hexadecimal digits represents one byte. Example: "129a2eff5c0b".
 * \param Buffer The output buffer.
 *
 * \return TRUE if the string was successfully converted, otherwise FALSE.
 */
BOOLEAN PhHexStringToBuffer(
    __in PPH_STRINGREF String,
    __out_bcount(String->Length / sizeof(WCHAR) / 2) PUCHAR Buffer
    )
{
    ULONG i;
    ULONG length;

    // The string must have an even length.
    if ((String->Length / sizeof(WCHAR)) & 1)
        return FALSE;

    length = String->Length / sizeof(WCHAR) / 2;

    for (i = 0; i < length; i++)
    {
        Buffer[i] =
            (UCHAR)(PhCharToInteger[(UCHAR)String->Buffer[i * 2]] << 4) +
            (UCHAR)PhCharToInteger[(UCHAR)String->Buffer[i * 2 + 1]];
    }

    return TRUE;
}

/**
 * Converts a byte array into a sequence of hexadecimal digits.
 *
 * \param Buffer The input buffer.
 * \param Length The number of bytes to convert.
 *
 * \return A string containing a sequence of hexadecimal digits.
 */
PPH_STRING PhBufferToHexString(
    __in PUCHAR Buffer,
    __in ULONG Length
    )
{
    PPH_STRING string;
    ULONG i;

    string = PhCreateStringEx(NULL, Length * 2 * sizeof(WCHAR));

    for (i = 0; i < Length; i++)
    {
        string->Buffer[i * 2] = PhIntegerToChar[Buffer[i] >> 4];
        string->Buffer[i * 2 + 1] = PhIntegerToChar[Buffer[i] & 0xf];
    }

    return string;
}

/**
 * Converts a string to an integer.
 *
 * \param String The string to process.
 * \param Base The base which the string uses to
 * represent the integer. The maximum value is
 * 69.
 * \param Integer The resulting integer.
 */
BOOLEAN PhpStringToInteger64(
    __in PPH_STRINGREF String,
    __in ULONG Base,
    __out PLONG64 Integer
    )
{
    LONG64 result;
    ULONG64 place;
    ULONG length;
    ULONG i;

    length = String->Length / sizeof(WCHAR);
    result = 0;
    place = 1;

    for (i = 0; i < length; i++)
    {
        WCHAR c;

        c = String->Buffer[length - i - 1];
        result += PhCharToInteger[(UCHAR)c] * place;
        place *= Base;
    }

    *Integer = result;

    return TRUE;
}

/**
 * Converts a string to an integer.
 *
 * \param String The string to process.
 * \param Base The base which the string uses to
 * represent the integer. The maximum value is
 * 69. If the parameter is 0, the base is inferred
 * from the string.
 * \param Integer The resulting integer.
 *
 * \remarks If \a Base is 0, the following prefixes
 * may be used to indicate bases:
 *
 * \li \c 0x Base 16.
 * \li \c 0o Base 8.
 * \li \c 0b Base 2.
 * \li \c 0t Base 3.
 * \li \c 0q Base 4.
 * \li \c 0w Base 12.
 * \li \c 0r Base 32.
 *
 * If there is no recognized prefix, base 10 is
 * used.
 */
BOOLEAN PhStringToInteger64(
    __in PPH_STRINGREF String,
    __in_opt ULONG Base,
    __out PLONG64 Integer
    )
{
    LONG64 result;
    PH_STRINGREF string;
    LONG64 sign = 1;
    ULONG base;

    if (Base > 69)
        return FALSE;

    string = *String;

    if (string.Length != 0 && (string.Buffer[0] == '-' || string.Buffer[0] == '+'))
    {
        if (string.Buffer[0] == '-')
            sign = -1;

        string.Buffer += 1;
        string.Length -= sizeof(WCHAR);
    }

    // If the caller specified a base, don't perform any
    // additional processing.

    if (Base)
    {
        base = Base;
    }
    else
    {
        base = 10;

        if (string.Length >= 2 * sizeof(WCHAR) && string.Buffer[0] == '0')
        {
            switch (string.Buffer[1])
            {
            case 'x':
            case 'X':
                base = 16;
                break;
            case 'o':
            case 'O':
                base = 8;
                break;
            case 'b':
            case 'B':
                base = 2;
                break;
            case 't': // ternary
            case 'T':
                base = 3;
                break;
            case 'q': // quaternary
            case 'Q':
                base = 4;
                break;
            case 'w': // base 12
            case 'W':
                base = 12;
                break;
            case 'r': // base 32
            case 'R':
                base = 32;
                break;
            }

            if (base != 10)
            {
                string.Buffer += 2;
                string.Length -= 2 * sizeof(WCHAR);
            }
        }
    }

    if (!PhpStringToInteger64(&string, base, &result))
        return FALSE;

    *Integer = sign * result;

    return TRUE;
}

/**
 * Converts an integer to a string.
 *
 * \param Integer The integer to process.
 * \param Base The base which the integer is
 * represented with. The maximum value is
 * 69. The base cannot be 1. If the parameter is 0,
 * the base used is 10.
 * \param Signed TRUE if \a Integer is a signed value,
 * otherwise FALSE.
 *
 * \return The resulting string, or NULL if an error
 * occurred.
 */
PPH_STRING PhIntegerToString64(
    __in LONG64 Integer,
    __in_opt ULONG Base,
    __in BOOLEAN Signed
    )
{
    PH_FORMAT format;

    if (Base == 1 || Base > 69)
        return NULL;

    if (Signed)
        PhInitFormatI64D(&format, Integer);
    else
        PhInitFormatI64U(&format, Integer);

    if (Base != 0)
    {
        format.Type |= FormatUseRadix;
        format.Radix = (UCHAR)Base;
    }

    return PhFormat(&format, 1, 0);
}

VOID PhPrintTimeSpan(
    __out_ecount(PH_TIMESPAN_STR_LEN_1) PWSTR Destination,
    __in ULONG64 Ticks,
    __in_opt ULONG Mode
    )
{
    switch (Mode)
    {
    case PH_TIMESPAN_HMSM:
        _snwprintf(
            Destination,
            PH_TIMESPAN_STR_LEN,
            L"%02I64u:%02I64u:%02I64u.%03I64u",
            PH_TICKS_PARTIAL_HOURS(Ticks),
            PH_TICKS_PARTIAL_MIN(Ticks),
            PH_TICKS_PARTIAL_SEC(Ticks),
            PH_TICKS_PARTIAL_MS(Ticks)
            );
        break;
    case PH_TIMESPAN_DHMS:
        _snwprintf(
            Destination,
            PH_TIMESPAN_STR_LEN,
            L"%I64u:%02I64u:%02I64u:%02I64u",
            PH_TICKS_PARTIAL_DAYS(Ticks),
            PH_TICKS_PARTIAL_HOURS(Ticks),
            PH_TICKS_PARTIAL_MIN(Ticks),
            PH_TICKS_PARTIAL_SEC(Ticks)
            );
        break;
    default:
        _snwprintf(
            Destination,
            PH_TIMESPAN_STR_LEN,
            L"%02I64u:%02I64u:%02I64u",
            PH_TICKS_PARTIAL_HOURS(Ticks),
            PH_TICKS_PARTIAL_MIN(Ticks),
            PH_TICKS_PARTIAL_SEC(Ticks)
            );
        break;
    }
}

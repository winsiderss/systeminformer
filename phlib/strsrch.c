/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2023
 *     jxy-s   2024
 *
 */

#include <ph.h>
#include <strsrch.h>

typedef struct _PH_STRING_SEARCH_CONEXT
{
    ULONG MinimumLength;
    BOOLEAN ExtendedCharSet;
    PPH_STRING_SEARCH_CALLBACK Callback;
    PVOID CallbackContext;
    WCHAR Buffer[PAGE_SIZE * 2];
} PH_STRING_SEARCH_CONEXT, *PPH_STRING_SEARCH_CONEXT;

BOOLEAN PhpSearchStrings(
    _In_ PPH_STRING_SEARCH_CONEXT Context,
    _In_reads_bytes_(Length) PBYTE Buffer,
    _In_ SIZE_T Length
    )
{
    BYTE byte = 0; // current byte
    BYTE byte1 = 0; // previous byte
    BYTE byte2 = 0; // byte before previous byte
    BOOLEAN printable = FALSE;
    BOOLEAN printable1 = FALSE;
    BOOLEAN printable2 = FALSE;
    ULONG length = 0;

    for (SIZE_T i = 0; i < Length; i++)
    {
        byte = Buffer[i];

        if (Context->ExtendedCharSet)
            printable = PhCharIsPrintableEx[byte];
        else
            printable = PhCharIsPrintable[byte];

        // To find strings System Informer uses a state table.
        // * byte2 - byte before previous byte
        // * byte1 - previous byte
        // * byte - current byte
        // * length - length of current string run
        //
        // The states are described below.
        //
        //    [byte2] [byte1] [byte] ...
        //    [char] means printable, [oth] means non-printable.
        //
        // 1. [char] [char] [char] ...
        //      (we're in a non-wide sequence)
        //      -> append char.
        if (printable2 && printable1 && printable)
        {
            if (length < RTL_NUMBER_OF(Context->Buffer))
                Context->Buffer[length++] = byte;
        }
        // 2. [char] [char] [oth] ...
        //      (we reached the end of a non-wide sequence, or we need to start a wide sequence)
        //      -> if current string is big enough, create result (non-wide).
        //         otherwise if byte = null, reset to new string with byte1 as first character.
        //         otherwise if byte != null, reset to new string.
        else if (printable2 && printable1 && !printable)
        {
            if (length >= Context->MinimumLength)
            {
                goto CreateResult;
            }
            else if (byte == 0)
            {
                length = 1;
                Context->Buffer[0] = byte1;
            }
            else
            {
                length = 0;
            }
        }
        // 3. [char] [oth] [char] ...
        //      (we're in a wide sequence, or could be starting one)
        //      -> (byte1 should = null) append char.
        else if (printable2 && !printable1 && printable)
        {
            if (length == 0 || byte1 == 0)
            {
                if (length < RTL_NUMBER_OF(Context->Buffer))
                    Context->Buffer[length++] = byte;
            }
            else
            {
                length = 0;
            }
        }
        // 4. [char] [oth] [oth] ...
        //      (we reached the end of a wide sequence)
        //      -> (byte1 should = null) if the current string is big enough, create result (wide).
        //         otherwise, reset to new string.
        else if (printable2 && !printable1 && !printable)
        {
            if (length >= Context->MinimumLength)
                goto CreateResult;
            else
                length = 0;
        }
        // 5. [oth] [char] [char] ...
        //      (we reached the end of a wide sequence, or we need to start a non-wide sequence)
        //      -> (excluding byte1) if the current string is big enough, create result (wide).
        //         otherwise, reset to new string with byte1 as first character and byte as
        //         second character.
        else if (!printable2 && printable1 && printable)
        {
            if (length >= Context->MinimumLength + 1) // length - 1 >= minimumLength but avoiding underflow
            {
                length--; // exclude byte1
                goto CreateResult;
            }
            else
            {
                length = 2;
                Context->Buffer[0] = byte1;
                Context->Buffer[1] = byte;
            }
        }
        // 6. [oth] [char] [oth] ...
        //      (we're in a wide sequence, or could be starting one)
        //      -> (byte2 and byte should = null) do nothing.
        else if (!printable2 && printable1 && !printable)
        {
            if (byte2 == 0 && byte == 0)
                NOTHING;
            else if (length == 1 && byte == 0) // starting a wide string
                NOTHING;
            else
                length = 0;
        }
        // 7. [oth] [oth] [char] ...
        //      (we're starting a sequence, but we don't know if it's a wide or non-wide sequence)
        //      -> append char.
        else if (!printable2 && !printable1 && printable)
        {
            if (length < RTL_NUMBER_OF(Context->Buffer))
                Context->Buffer[length++] = byte;
        }
        // 8. [oth] [oth] [oth] ...
        //      (nothing)
        //      -> do nothing.
        else if (!printable2 && !printable1 && !printable)
        {
            NOTHING;
        }

        goto AfterCreateResult;

CreateResult:
        {
            PH_STRING_SEARCH_RESULT result;
            ULONG lengthInBytes;
            ULONG bias;
            BOOLEAN isWide;
            BOOLEAN isFinished;

            lengthInBytes = length;
            bias = 0;
            isWide = FALSE;

            if (printable1 == printable) // determine if string was wide (refer to state table, 4 and 5)
            {
                isWide = TRUE;
                lengthInBytes *= 2;
            }

            if (printable) // byte1 excluded (refer to state table, 5)
            {
                bias = 1;
            }

            result.Unicode = isWide;
            result.Address = PTR_ADD_OFFSET(Buffer, i - bias - lengthInBytes);
            result.Length = lengthInBytes;
            result.String = PhCreateStringEx(
                Context->Buffer,
                2 * min(length, RTL_NUMBER_OF(Context->Buffer))
                );

            isFinished = Context->Callback(&result, Context->CallbackContext);

            PhClearReference(&result.String);
            length = 0;

            if (isFinished)
                return TRUE;
        }
AfterCreateResult:

        byte2 = byte1;
        byte1 = byte;
        printable2 = printable1;
        printable1 = printable;
    }

    return FALSE;
}

NTSTATUS PhSearchStrings(
    _In_ ULONG MinimumLength,
    _In_ BOOLEAN ExtendedCharSet,
    _In_ PPH_STRING_SEARCH_NEXT_BUFFER NextBuffer,
    _In_ PPH_STRING_SEARCH_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PPH_STRING_SEARCH_CONEXT context;
    PVOID buffer;
    SIZE_T length;

    if (!MinimumLength)
        return STATUS_INVALID_PARAMETER;

    context = PhAllocateZero(sizeof(PH_STRING_SEARCH_CONEXT));
    context->MinimumLength = MinimumLength;
    context->ExtendedCharSet = ExtendedCharSet;
    context->Callback = Callback;
    context->CallbackContext = Context;

    buffer = NULL;
    length = 0;

    while (NT_SUCCESS(status = NextBuffer(&buffer, &length, Context)))
    {
        if (!length)
            break;

        if (PhpSearchStrings(context, buffer, length))
            break;
    }

    PhFree(context);

    return status;
}

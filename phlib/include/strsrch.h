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

#ifndef _PH_MEMSRCH_H
#define _PH_MEMSRCH_H

EXTERN_C_START

typedef struct _PH_STRING_SEARCH_RESULT
{
    PVOID Address;
    SIZE_T Length;
    PPH_STRING String;
    BOOLEAN Unicode;
} PH_STRING_SEARCH_RESULT, *PPH_STRING_SEARCH_RESULT;

_Function_class_(PH_STRING_SEARCH_CALLBACK)
typedef
_Must_inspect_result_
BOOLEAN
NTAPI
PH_STRING_SEARCH_CALLBACK(
    _In_ PPH_STRING_SEARCH_RESULT Result,
    _In_opt_ PVOID Context
    );
typedef PH_STRING_SEARCH_CALLBACK* PPH_STRING_SEARCH_CALLBACK;

_Function_class_(PH_STRING_SEARCH_NEXT_BUFFER)
typedef
_Must_inspect_result_
NTSTATUS
NTAPI
PH_STRING_SEARCH_NEXT_BUFFER(
    _Inout_bytecount_(*Length) PVOID* Buffer,
    _Out_ PSIZE_T Length,
    _In_opt_ PVOID Context
    );
typedef PH_STRING_SEARCH_NEXT_BUFFER* PPH_STRING_SEARCH_NEXT_BUFFER;

PHLIBAPI
NTSTATUS
NTAPI
PhSearchStrings(
    _In_ ULONG MinimumLength,
    _In_ BOOLEAN ExtendedUnicode,
    _In_ PPH_STRING_SEARCH_NEXT_BUFFER NextBuffer,
    _In_ PPH_STRING_SEARCH_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

EXTERN_C_END

#endif

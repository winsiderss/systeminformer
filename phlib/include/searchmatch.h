/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2012-2026
 *
 */

#ifndef _PH_SEARCHMATCH_H
#define _PH_SEARCHMATCH_H

//
// Private search matching engine shared by the search controls (searchbox.c, searchboxextended.cpp).
// The MatchHandle passed to search callbacks is a pointer to a PH_SEARCH_MATCH_STATE.
//

#include <searchbox.h>

EXTERN_C_START

typedef struct _PH_SEARCH_MATCH_STATE
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG UseSearchPointer : 1;
            ULONG CaseActive : 1;
            ULONG RegexActive : 1;
            ULONG FuzzyActive : 1;
            ULONG RegexError : 1;
            ULONG Spare : 27;
        };
    };

    PH_STRINGREF SearchboxText;
    PPH_STRING SearchboxTextString;

    ULONG64 SearchPointer;
    LONG SearchboxRegexError;
    SIZE_T SearchboxRegexErrorOffset;
    PVOID SearchboxRegexCode; // pcre2_code*
    PVOID SearchboxRegexMatchData; // pcre2_match_data*
} PH_SEARCH_MATCH_STATE, *PPH_SEARCH_MATCH_STATE;

VOID PhSearchMatchUpdateRegex(
    _Inout_ PPH_SEARCH_MATCH_STATE State
    );

BOOLEAN PhSearchMatchSetText(
    _Inout_ PPH_SEARCH_MATCH_STATE State,
    _In_ PPH_STRING Text,
    _In_ BOOLEAN Force
    );

ULONG_PTR PhSearchMatchGetHandle(
    _In_ PPH_SEARCH_MATCH_STATE State
    );

VOID PhSearchMatchDelete(
    _Inout_ PPH_SEARCH_MATCH_STATE State
    );

BOOLEAN PhSearchMatchString(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCPH_STRINGREF Text
    );

BOOLEAN PhSearchMatchStringEx(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCPH_STRINGREF Text,
    _Out_writes_to_opt_(MaximumRanges, *RangeCount) PPH_SEARCHCONTROL_MATCH_RANGE Ranges,
    _In_ ULONG MaximumRanges,
    _Out_opt_ PULONG RangeCount
    );

BOOLEAN PhSearchMatchPointer(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer
    );

BOOLEAN PhSearchMatchPointerRange(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer,
    _In_ SIZE_T Size
    );

EXTERN_C_END

#endif

/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2012-2026
 *     jxy-s   2023-2024
 *
 */

#include <ph.h>
#include <searchmatch.h>
#include <thirdparty.h>

/**
 * Recompiles the regular expression for the current search text.
 *
 * \param State The search match state.
 */
VOID PhSearchMatchUpdateRegex(
    _Inout_ PPH_SEARCH_MATCH_STATE State
    )
{
    ULONG flags;
    PCRE2_SIZE errorOffset = 0;

    State->RegexError = FALSE;
    State->SearchboxRegexError = 0;
    State->SearchboxRegexErrorOffset = 0;

    if (State->SearchboxRegexCode)
    {
        pcre2_code_free(State->SearchboxRegexCode);
        State->SearchboxRegexCode = NULL;
    }

    if (State->SearchboxRegexMatchData)
    {
        pcre2_match_data_free(State->SearchboxRegexMatchData);
        State->SearchboxRegexMatchData = NULL;
    }

    if (!State->RegexActive || State->SearchboxText.Length == 0)
        return;

    if (State->CaseActive)
        flags = PCRE2_DOTALL;
    else
        flags = PCRE2_CASELESS | PCRE2_DOTALL;

    State->SearchboxRegexCode = pcre2_compile(
        State->SearchboxText.Buffer,
        State->SearchboxText.Length / sizeof(WCHAR),
        flags,
        &State->SearchboxRegexError,
        &errorOffset,
        NULL
        );
    State->SearchboxRegexErrorOffset = errorOffset;

    if (!State->SearchboxRegexCode)
    {
        State->RegexError = TRUE;
        return;
    }

    State->SearchboxRegexMatchData = pcre2_match_data_create_from_pattern(
        State->SearchboxRegexCode,
        NULL
        );
}

/**
 * Updates the search text.
 *
 * \param State The search match state.
 * \param Text The new search text. The state takes its own reference.
 * \param Force TRUE to update even if the text has not changed.
 * \return TRUE if the text was updated, otherwise FALSE.
 */
BOOLEAN PhSearchMatchSetText(
    _Inout_ PPH_SEARCH_MATCH_STATE State,
    _In_ PPH_STRING Text,
    _In_ BOOLEAN Force
    )
{
    if (!Force && PhEqualStringRef(&Text->sr, &State->SearchboxText, FALSE))
        return FALSE;

    PhSetReference(&State->SearchboxTextString, Text);
    State->SearchboxText = Text->sr;
    State->UseSearchPointer = PhStringToUInt64(&State->SearchboxText, 0, &State->SearchPointer);

    return TRUE;
}

/**
 * Gets the match handle passed to search callbacks.
 *
 * \param State The search match state.
 * \return The match handle, or 0 when every item should match (no filter).
 */
ULONG_PTR PhSearchMatchGetHandle(
    _In_ PPH_SEARCH_MATCH_STATE State
    )
{
    if (State->SearchboxText.Length == 0 || (State->RegexActive && !State->SearchboxRegexCode))
        return 0;

    return (ULONG_PTR)State;
}

/**
 * Frees the resources owned by the search match state.
 *
 * \param State The search match state.
 */
VOID PhSearchMatchDelete(
    _Inout_ PPH_SEARCH_MATCH_STATE State
    )
{
    if (State->SearchboxTextString)
    {
        PhDereferenceObject(State->SearchboxTextString);
        State->SearchboxTextString = NULL;
    }

    if (State->SearchboxRegexCode)
    {
        pcre2_code_free(State->SearchboxRegexCode);
        State->SearchboxRegexCode = NULL;
    }

    if (State->SearchboxRegexMatchData)
    {
        pcre2_match_data_free(State->SearchboxRegexMatchData);
        State->SearchboxRegexMatchData = NULL;
    }

    PhInitializeEmptyStringRef(&State->SearchboxText);
}

/**
 * Checks if a text string matches the current search criteria.
 *
 * \param MatchHandle A handle used for matching, provided by the search callback.
 * \param Text The text to match against the search criteria.
 * \return TRUE if the text matches the search criteria, otherwise FALSE.
 */
BOOLEAN PhSearchMatchString(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCPH_STRINGREF Text
    )
{
    PPH_SEARCH_MATCH_STATE state = (PPH_SEARCH_MATCH_STATE)MatchHandle;

    if (!state)
        return FALSE;

    if (state->FuzzyActive)
    {
        return PhStringFuzzyMatch(&state->SearchboxText, Text, !state->CaseActive);
    }
    else if (state->RegexActive)
    {
        if (pcre2_match(
            state->SearchboxRegexCode,
            Text->Buffer,
            Text->Length / sizeof(WCHAR),
            0,
            0,
            state->SearchboxRegexMatchData,
            NULL
            ) >= 0)
        {
            return TRUE;
        }
    }
    else if (state->CaseActive)
    {
        if (PhFindStringInStringRef(Text, &state->SearchboxText, FALSE) != MAXULONG_PTR)
            return TRUE;
    }
    else
    {
        if (PhFindStringInStringRef(Text, &state->SearchboxText, TRUE) != MAXULONG_PTR)
            return TRUE;
    }

    return FALSE;
}

/**
 * Checks if a text string matches the current search criteria and retrieves the match ranges.
 *
 * \param MatchHandle A handle used for matching, provided by the search callback.
 * \param Text The text to match against the search criteria.
 * \param Ranges A buffer that receives the match ranges.
 * \param MaximumRanges The maximum number of ranges that the buffer can hold.
 * \param RangeCount A variable which receives the number of match ranges.
 * \return TRUE if the text matches the search criteria, otherwise FALSE.
 */
BOOLEAN PhSearchMatchStringEx(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCPH_STRINGREF Text,
    _Out_writes_to_opt_(MaximumRanges, *RangeCount) PPH_SEARCHCONTROL_MATCH_RANGE Ranges,
    _In_ ULONG MaximumRanges,
    _Out_opt_ PULONG RangeCount
    )
{
    PPH_SEARCH_MATCH_STATE state = (PPH_SEARCH_MATCH_STATE)MatchHandle;
    ULONG rangeCount = 0;
    BOOLEAN result = FALSE;

    if (RangeCount)
        *RangeCount = 0;

    if (!state || state->SearchboxText.Length == 0)
        return FALSE;

    if (state->FuzzyActive)
    {
        // Fuzzy matches have no contiguous span; report the match without ranges.
        result = PhStringFuzzyMatch(&state->SearchboxText, Text, !state->CaseActive);
    }
    else if (state->RegexActive)
    {
        if (pcre2_match(
            state->SearchboxRegexCode,
            Text->Buffer,
            Text->Length / sizeof(WCHAR),
            0,
            0,
            state->SearchboxRegexMatchData,
            NULL
            ) >= 0)
        {
            PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(state->SearchboxRegexMatchData);

            result = TRUE;

            if (Ranges && MaximumRanges != 0 && ovector[1] > ovector[0])
            {
                Ranges[0].Start = (ULONG)ovector[0];
                Ranges[0].Length = (ULONG)(ovector[1] - ovector[0]);
                rangeCount = 1;
            }
        }
    }
    else
    {
        BOOLEAN ignoreCase = !state->CaseActive;
        ULONG searchLength = (ULONG)(state->SearchboxText.Length / sizeof(WCHAR));
        PH_STRINGREF remaining = *Text;
        ULONG offset = 0;
        SIZE_T index;

        while ((index = PhFindStringInStringRef(&remaining, &state->SearchboxText, ignoreCase)) != MAXULONG_PTR)
        {
            result = TRUE;

            if (!Ranges || rangeCount >= MaximumRanges)
                break;

            Ranges[rangeCount].Start = offset + (ULONG)index;
            Ranges[rangeCount].Length = searchLength;
            rangeCount++;

            offset += (ULONG)index + searchLength;
            remaining.Buffer += index + searchLength;
            remaining.Length -= (index + searchLength) * sizeof(WCHAR);
        }
    }

    if (RangeCount)
        *RangeCount = rangeCount;

    return result;
}

/**
 * Checks if a pointer matches the current search criteria.
 *
 * \param MatchHandle A handle used for matching, provided by the search callback.
 * \param Pointer The pointer to match against the search criteria.
 * \return TRUE if the pointer matches the search criteria, otherwise FALSE.
 */
BOOLEAN PhSearchMatchPointer(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer
    )
{
    PPH_SEARCH_MATCH_STATE state = (PPH_SEARCH_MATCH_STATE)MatchHandle;

    if (!Pointer || !state || !state->UseSearchPointer)
        return FALSE;

    return ((ULONG64)Pointer == state->SearchPointer);
}

/**
 * Checks if any pointer in a given range matches the current search criteria.
 *
 * \param MatchHandle A handle used for matching, provided by the search callback.
 * \param Pointer The start of the pointer range.
 * \param Size The size of the pointer range, in bytes.
 * \return TRUE if a pointer in the range matches the search criteria, otherwise FALSE.
 */
BOOLEAN PhSearchMatchPointerRange(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer,
    _In_ SIZE_T Size
    )
{
    PPH_SEARCH_MATCH_STATE state = (PPH_SEARCH_MATCH_STATE)MatchHandle;
    PVOID pointerEnd;

    if (!state || !state->UseSearchPointer)
        return FALSE;

    pointerEnd = PTR_ADD_OFFSET(Pointer, Size);

    return ((state->SearchPointer >= (ULONG64)Pointer) &&
            (state->SearchPointer < (ULONG64)pointerEnd));
}

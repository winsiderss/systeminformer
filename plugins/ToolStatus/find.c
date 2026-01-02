/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2011-2023
 *
 */

#include "toolstatus.h"

ULONG FindDialogMessage = ULONG_MAX;
static FINDREPLACE FindDialogData = { 0 };
static PH_STRINGREF FindDialogExpression = { 0 };
static WCHAR FindDialogText[128] = L"";
static HWND FindDialogHandle = NULL;
static ULONG LastFindFlatIndex = ULONG_MAX; // last matched flat index within current tree
static HWND LastFindTreeHandle = NULL;
static BOOLEAN LastFindMatchCase = FALSE;
static BOOLEAN LastFindMatchWholeWord = FALSE;
static BOOLEAN LastFindSearchDown = TRUE;

/**
 * Determines whether a character represents a word boundary.
 *
 * This function is used by the "Match Whole Word" search feature to identify
 * characters that delimit words. Word boundaries include whitespace, punctuation,
 * and various special characters commonly used as separators in text.
 *
 * \param ch The wide character to check.
 * \return TRUE if the character is a word boundary; otherwise, FALSE.
 */
static BOOLEAN IsWordBoundary(WCHAR ch)
{
    return ch == L' ' || ch == L'\t' || ch == L'\n' || ch == L'\r' || 
           ch == L',' || ch == L'.' || ch == L';' || ch == L':' ||
           ch == L'!' || ch == L'?' || ch == L'(' || ch == L')' ||
           ch == L'[' || ch == L']' || ch == L'{' || ch == L'}' ||
           ch == L'<' || ch == L'>' || ch == L'/' || ch == L'\\' ||
           ch == L'|' || ch == L'-' || ch == L'_' || ch == L'=' ||
           ch == L'+' || ch == L'*' || ch == L'&' || ch == L'^' ||
           ch == L'%' || ch == L'$' || ch == L'#' || ch == L'@' ||
           ch == L'~' || ch == L'`' || ch == L'\'' || ch == L'"';
}

/**
 * Determines whether a haystack string contains a substring with optional case, whole-word, and wildcard matching.
 *
 * This function searches for a substring within a PH_STRINGREF haystack and optionally enforces
 * case-sensitive matching and whole-word matching. It also supports wildcard patterns using the
 * phlib wildcard matching functions (PhIsNameInExpression).
 *
 * Supported wildcards:
 * - '*' matches zero or more characters
 * - '?' matches exactly one character
 * - DOS_STAR, DOS_QM, DOS_DOT are also supported by the underlying PhIsNameInExpression
 *
 * \param Haystack A pointer to a PH_STRINGREF structure containing the string to search within.
 * \param SubString A pointer to a null-terminated wide-character string to search for (may contain wildcards).
 * \param MatchCase If TRUE, performs case-sensitive matching; otherwise, case-insensitive.
 * \param MatchWholeWord If TRUE, ensures the substring is bounded by word boundaries (only applies to non-wildcard searches).
 * \return TRUE if the substring is found (and satisfies whole-word constraints if enabled); otherwise, FALSE.
 * \remarks
 * - Returns FALSE if Haystack is NULL, SubString is NULL, or SubString is empty.
 * - If SubString contains wildcards (* or ?), uses PhIsNameInExpression for pattern matching.
 * - Wildcard matching ignores the MatchWholeWord parameter.
 * - For non-wildcard searches, uses PhFindStringInStringRefZ and optionally validates word boundaries.
 * - When MatchWholeWord is TRUE (non-wildcard), validates that characters immediately before and after
 *   the match position are word boundaries (as defined by IsWordBoundary).
 */
static BOOLEAN StringContains(
    _In_ PCPH_STRINGREF Haystack,
    _In_ PCPH_STRINGREF SubString,
    _In_ BOOLEAN MatchCase,
    _In_ BOOLEAN MatchWholeWord
    )
{
    if (!Haystack || !SubString || !SubString)
        return FALSE;

    // Check if the pattern contains wildcards
    if (PhDoesNameContainWildCards(SubString))
    {
        return PhIsNameInExpression(SubString, Haystack, !MatchCase);
    }

    // Non-wildcard search with whole word support
    SIZE_T length = SubString->Length;// PhCountStringZ(SubString);
    SIZE_T index = PhFindStringInStringRef(Haystack, SubString, MatchCase);
    
    if (index == SIZE_MAX)
        return FALSE;

    if (!MatchWholeWord)
        return TRUE;

    // Check word boundaries
    SIZE_T haystack = Haystack->Length / sizeof(WCHAR);
    
    // Check start boundary
    if (index > 0 && !IsWordBoundary(Haystack->Buffer[index - 1]))
        return FALSE;
    
    // Check end boundary
    if (index + length < haystack && !IsWordBoundary(Haystack->Buffer[index + length]))
        return FALSE;

    return TRUE;
}

/**
 * Searches for a text match within a tree node's visible columns.
 *
 * This function enumerates all visible columns in a TreeNew control and checks
 * whether any cell text contains the specified search string. It supports both
 * case-sensitive and whole-word matching options.
 *
 * \param TreeHandle A handle to the TreeNew control containing the node.
 * \param Node A pointer to the PH_TREENEW_NODE structure to search within.
 * \param String A pointer to a null-terminated wide-character string to search for.
 * \param MatchCase If TRUE, performs case-sensitive matching; otherwise, case-insensitive.
 * \param MatchWholeWord If TRUE, ensures the search string is bounded by word boundaries.
 * \return TRUE if a match is found in any visible column; otherwise, FALSE.
 * \remarks
 * - Returns FALSE if TreeHandle, Node, or String is NULL, or if String is empty.
 * - Only searches cells in visible columns.
 * - For performance, limits cell text length checks to 0x1000 characters.
 * - Uses a 512-character buffer for cell text; longer text is truncated.
 * - Calls StringContains() to perform the actual text matching with the specified options.
 * - Memory allocated for displayToId and displayToText is freed before returning.
 */
BOOLEAN FindMatchInNode(
    _In_ HWND TreeHandle,
    _In_ PPH_TREENEW_NODE Node,
    _In_ PCPH_STRINGREF String,
    _In_ BOOLEAN MatchCase,
    _In_ BOOLEAN MatchWholeWord
    )
{
    PPH_TREENEW_COLUMN fixedColumn;
    ULONG numberOfColumns;
    PULONG displayToId;
    ULONG i;
    PH_TREENEW_COLUMN column;

    if (!TreeHandle || !Node || !String || !String->Buffer || !String->Buffer[0])
        return FALSE;

    // Enumerate visible columns

    fixedColumn = TreeNew_GetFixedColumn(TreeHandle);
    numberOfColumns = TreeNew_GetVisibleColumnCount(TreeHandle);

    displayToId = PhAllocate(numberOfColumns * sizeof(ULONG));

    if (fixedColumn)
    {
        TreeNew_GetColumnOrderArray(TreeHandle, numberOfColumns - 1, displayToId + 1);
        displayToId[0] = fixedColumn->Id;
    }
    else
    {
        TreeNew_GetColumnOrderArray(TreeHandle, numberOfColumns, displayToId);
    }

    for (i = 0; i < numberOfColumns; i++)
    {
        if (TreeNew_GetColumn(TreeHandle, displayToId[i], &column))
        {
            if (column.Visible)
            {
                PH_TREENEW_GET_CELL_TEXT getCellText;

                memset(&getCellText, 0, sizeof(getCellText));
                getCellText.Node = Node;
                getCellText.Id = column.Id;
                TreeNew_GetCellText(TreeHandle, &getCellText);

                if (getCellText.Text.Buffer && getCellText.Text.Length)
                {
                    SIZE_T length = getCellText.Text.Length / sizeof(WCHAR);

                    if (length < 0x1000)
                    {
                        PH_STRINGREF string;
                        WCHAR buffer[512];

                        if (length >= RTL_NUMBER_OF(buffer))
                            length = RTL_NUMBER_OF(buffer) - 1;

                        wcsncpy_s(buffer, RTL_NUMBER_OF(buffer), getCellText.Text.Buffer, length);
                        buffer[length] = UNICODE_NULL;

                        string.Buffer = buffer;
                        string.Length = length * sizeof(WCHAR);

                        if (StringContains(&string, String, MatchCase, MatchWholeWord))
                        {
                            PhFree(displayToId);
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

    PhFree(displayToId);

    return FALSE;
}

/**
 * Executes a search operation to find the next match in the current TreeNew control.
 *
 * This function searches through visible nodes in the active TreeNew control to find
 * the next occurrence of the search text (stored in FindDialogText). It supports both
 * forward and backward searching, with optional wrapping from the last/first node back
 * to the first/last node.
 *
 * \param RestartFromTop If TRUE, starts the search from the beginning (or end, depending
 * on search direction), ignoring the last search position.
 * If FALSE, continues from the last found position (LastFindFlatIndex).
 * \param String A pointer to a null-terminated wide-character string to search for.
 * \param MatchCase If TRUE, performs case-sensitive matching; otherwise, case-insensitive.
 * \param MatchWholeWord If TRUE, ensures the search string is bounded by word boundaries.
 * \return TRUE if a match is found and the node is selected and made visible; otherwise, FALSE.
 * \remarks
 * - Returns FALSE if no TreeNew control is active, if FindDialogText is empty, or if there are no nodes.
 * - Search direction is determined by LastFindSearchDown (TRUE = forward, FALSE = backward).
 * - When continuing from a previous search (RestartFromTop = FALSE):
 *   - Forward search: starts from (LastFindFlatIndex + 1), wraps to beginning if needed.
 *   - Backward search: starts from (LastFindFlatIndex - 1), wraps to end if needed.
 * - When RestartFromTop = TRUE, search starts from index 0 (forward) or (flatCount - 1) (backward).
 * - Search wraps around once: forward searches continue from 0 to startIndex; backward searches
 *   continue from (flatCount - 1) down to startIndex.
 * - Uses LastFindMatchCase and LastFindMatchWholeWord flags for matching options.
 * - Updates LastFindFlatIndex and LastFindTreeHandle when a match is found.
 * - Skips invisible nodes during the search.
 * - Calls FindMatchInNode() to perform the actual text matching against visible column cells.
 * - On success, the matched node is focused, selected, and scrolled into view via
 *   TreeNew_FocusMarkSelectNode() and TreeNew_EnsureVisible().
 */
BOOLEAN ExecuteFindNext(
    _In_ BOOLEAN RestartFromTop,
    _In_ PCPH_STRINGREF String,
    _In_ BOOLEAN MatchCase,
    _In_ BOOLEAN MatchWholeWord
    )
{
    HWND tnHandle;
    ULONG flatCount;
    ULONG startIndex;
    LONG increment;

    if (!(tnHandle = GetCurrentTreeNewHandle()))
        return FALSE;

    if (FindDialogText[0] == UNICODE_NULL)
        return FALSE;

    if (!(flatCount = TreeNew_GetFlatNodeCount(tnHandle)))
        return FALSE;

    startIndex = 0;
    increment = LastFindSearchDown ? 1 : -1;

    if (
        !RestartFromTop &&
        LastFindTreeHandle == tnHandle &&
        LastFindFlatIndex != ULONG_MAX
        )
    {
        if (LastFindSearchDown && LastFindFlatIndex + 1 < flatCount)
            startIndex = LastFindFlatIndex + 1;
        else if (!LastFindSearchDown && LastFindFlatIndex > 0)
            startIndex = LastFindFlatIndex - 1;
        else if (LastFindSearchDown)
            startIndex = 0; // wrap to beginning
        else
            startIndex = flatCount - 1; // wrap to end
    }
    else
    {
        startIndex = LastFindSearchDown ? 0 : flatCount - 1;
    }

    // Search in the specified direction
    if (LastFindSearchDown)
    {
        // Search down from startIndex to end
        for (ULONG i = startIndex; i < flatCount; i++)
        {
            PPH_TREENEW_NODE node = TreeNew_GetFlatNode(tnHandle, i);

            if (!node || !node->Visible)
                continue;

            if (FindMatchInNode(tnHandle, node, String, MatchCase, MatchWholeWord))
            {
                TreeNew_FocusMarkSelectNode(tnHandle, node);
                TreeNew_EnsureVisible(tnHandle, node);
                LastFindFlatIndex = i;
                LastFindTreeHandle = tnHandle;
                return TRUE;
            }
        }

        // Wrap around to beginning
        if (!RestartFromTop && startIndex != 0)
        {
            for (ULONG i = 0; i < startIndex; i++)
            {
                PPH_TREENEW_NODE node = TreeNew_GetFlatNode(tnHandle, i);

                if (!node || !node->Visible)
                    continue;

                if (FindMatchInNode(tnHandle, node, String, MatchCase, MatchWholeWord))
                {
                    TreeNew_FocusMarkSelectNode(tnHandle, node);
                    TreeNew_EnsureVisible(tnHandle, node);
                    LastFindFlatIndex = i;
                    LastFindTreeHandle = tnHandle;
                    return TRUE;
                }
            }
        }
    }
    else
    {
        // Search up from startIndex to beginning
        for (LONG i = (LONG)startIndex; i >= 0; i--)
        {
            PPH_TREENEW_NODE node = TreeNew_GetFlatNode(tnHandle, i);

            if (!node || !node->Visible)
                continue;

            if (FindMatchInNode(tnHandle, node, String, MatchCase, MatchWholeWord))
            {
                TreeNew_FocusMarkSelectNode(tnHandle, node);
                TreeNew_EnsureVisible(tnHandle, node);
                LastFindFlatIndex = i;
                LastFindTreeHandle = tnHandle;
                return TRUE;
            }
        }

        // Wrap around to end
        if (!RestartFromTop && startIndex != flatCount - 1)
        {
            for (LONG i = (LONG)(flatCount - 1); i > (LONG)startIndex; i--)
            {
                PPH_TREENEW_NODE node = TreeNew_GetFlatNode(tnHandle, i);

                if (!node || !node->Visible)
                    continue;

                if (FindMatchInNode(tnHandle, node, String, MatchCase, MatchWholeWord))
                {
                    TreeNew_FocusMarkSelectNode(tnHandle, node);
                    TreeNew_EnsureVisible(tnHandle, node);
                    LastFindFlatIndex = i;
                    LastFindTreeHandle = tnHandle;
                    return TRUE;
                }
            }
        }
    }

    //MessageBeep(MB_ICONINFORMATION);

    return FALSE;
}

/**
 * Handles messages from the system Find dialog to execute search operations.
 * \param lParam An LPARAM containing a pointer to a FINDREPLACE structure from the Find dialog.
 */
VOID FindDialogHandleFindMessage(
    _In_ LPARAM lParam
    )
{
    LPFINDREPLACE fr = (LPFINDREPLACE)lParam;

    if (FlagOn(fr->Flags, FR_DIALOGTERM))
    {
        FindDialogHandle = NULL;
    }
    else if (FlagOn(fr->Flags, FR_FINDNEXT))
    {
        BOOLEAN matchCase = !!FlagOn(fr->Flags, FR_MATCHCASE);
        BOOLEAN matchWholeWord = !!FlagOn(fr->Flags, FR_WHOLEWORD);
        BOOLEAN searchDown = !!FlagOn(fr->Flags, FR_DOWN);
        BOOLEAN searchParamsChanged = FALSE;

        if (!FlagOn(fr->Flags, FR_FINDNEXT))
            return;

        // If search text changed, reset index so we start fresh
        if (!PhEqualStringZ(FindDialogText, fr->lpstrFindWhat, FALSE))
        {
            wcsncpy_s(FindDialogText, RTL_NUMBER_OF(FindDialogText), fr->lpstrFindWhat, _TRUNCATE);
            PhInitializeStringRefLongHint(&FindDialogExpression, FindDialogText);
            searchParamsChanged = TRUE;
        }
        else
        {
            PhInitializeStringRefLongHint(&FindDialogExpression, FindDialogText);
        }

        // If match case changed, reset index
        if (LastFindMatchCase != matchCase)
        {
            LastFindMatchCase = matchCase;
            searchParamsChanged = TRUE;
        }

        // If match whole word changed, reset index
        if (LastFindMatchWholeWord != matchWholeWord)
        {
            LastFindMatchWholeWord = matchWholeWord;
            searchParamsChanged = TRUE;
        }

        // Update search direction (don't reset on direction change)
        LastFindSearchDown = searchDown;

        if (searchParamsChanged)
        {
            LastFindFlatIndex = ULONG_MAX;
            LastFindTreeHandle = NULL;
        }

        ExecuteFindNext(FALSE, &FindDialogExpression, LastFindMatchCase, LastFindMatchWholeWord);
    }
}

/**
 * Displays the system Find dialog for searching TreeNew control content.
 *
 * \param OwnerWindow A handle to the window that will own the Find dialog.
 * \remarks
 * - On first invocation, registers the FINDMSGSTRING message used for dialog callbacks.
 * - Initializes FindDialogData structure with search buffer (FindDialogText) and owner window.
 * - The dialog is modeless; messages are handled via FindDialogHandleFindMessage() when
 *   the registered FindDialogMessage is received by the owner window's message loop.
 * - If FindDialogHandle is non-NULL, the function assumes the dialog is already open and
 *   calls SetForegroundWindow() to activate it rather than creating a new instance.
 * - The dialog remains open until explicitly closed by the user or terminated programmatically.
 * - Search state (FindDialogText, LastFindMatchCase, LastFindMatchWholeWord, etc.) persists
 *   across dialog invocations to maintain search context.
 */
VOID ShowFindDialog(
    _In_ HWND OwnerWindow
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&FindTextW) FindText_I = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"comdlg32.dll"))
        {
            FindText_I = PhGetProcedureAddress(baseAddress, "FindTextW", 0);
        }

        if (FindDialogMessage == ULONG_MAX)
        {
            FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!FindText_I)
    {
        PhShowStatus(OwnerWindow, L"Unable to display Find dialog.", 0, ERROR_PROC_NOT_FOUND);
        return;
    }

    if (FindDialogHandle) // already open – bring to front
    {
        SetForegroundWindow(FindDialogHandle);
        return;
    }

    ZeroMemory(&FindDialogData, sizeof(FindDialogData));
    FindDialogData.lStructSize = sizeof(FindDialogData);
    FindDialogData.Flags = FR_DOWN;
    FindDialogData.hwndOwner = OwnerWindow;
    FindDialogData.lpstrFindWhat = FindDialogText;
    FindDialogData.wFindWhatLen = (WORD)RTL_NUMBER_OF(FindDialogText);

    FindDialogHandle = FindText_I(&FindDialogData);
}


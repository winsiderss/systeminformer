/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#ifndef _USERNOTESINTF_H
#define _USERNOTESINTF_H

#define USERNOTES_PLUGIN_NAME L"UserNotes"
#define USERNOTES_INTERFACE_VERSION 1

/**
 * What the user has saved against a process.
 *
 * \remarks UserNotes keys its entries on the process's file name or on its whole command line, and
 * the command line entry wins where both exist. MatchedCommandLine says which one answered, which
 * matters because editing the wrong one leaves the other in force.
 *
 * The fields that hold a saved setting have their own "nothing saved" value rather than a separate
 * flag, matching how the database stores them: the priority fields are one greater than the value
 * they carry so that zero means unset, the affinity mask is zero and the colour is ULONG_MAX.
 */
typedef struct _USERNOTES_PROCESS_NOTES
{
    BOOLEAN MatchedCommandLine; // The entry that answered is keyed on the command line.
    BOOLEAN Collapse;
    BOOLEAN Boost;
    BOOLEAN Efficiency;

    PPH_STRING Comment;         // Referenced; the caller dereferences it. NULL when there is none.
    ULONG PriorityClass;        // 0 when nothing is saved.
    ULONG IoPriorityPlusOne;    // 0 when nothing is saved, else the I/O priority plus one.
    ULONG PagePriorityPlusOne;  // 0 when nothing is saved, else the page priority plus one.
    ULONG BackColor;            // ULONG_MAX when nothing is saved.
    KAFFINITY AffinityMask;     // 0 when nothing is saved.
} USERNOTES_PROCESS_NOTES, *PUSERNOTES_PROCESS_NOTES;

/**
 * Copies out what is saved against a process.
 *
 * \param ProcessItem The process to look up.
 * \param Notes The copied entry. Not written when the process has no entry.
 * \return TRUE when an entry was found.
 */
typedef BOOLEAN (NTAPI* PUSERNOTES_GET_PROCESS_NOTES)(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _Out_ PUSERNOTES_PROCESS_NOTES Notes
    );

/**
 * Sets or clears the comment saved against a process.
 *
 * \param ProcessItem The process to annotate.
 * \param Comment The comment, or NULL or empty to remove it.
 * \param MatchCommandLine Save against this process's whole command line rather than its file name.
 * \return TRUE if the database was changed and written.
 *
 * \remarks Writes the database to disk, as the properties page does.
 */
typedef BOOLEAN (NTAPI* PUSERNOTES_SET_PROCESS_COMMENT)(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_opt_ PCPH_STRINGREF Comment,
    _In_ BOOLEAN MatchCommandLine
    );

typedef struct _USERNOTES_INTERFACE
{
    ULONG Version;
    PUSERNOTES_GET_PROCESS_NOTES GetProcessNotes;
    PUSERNOTES_SET_PROCESS_COMMENT SetProcessComment;
} USERNOTES_INTERFACE, *PUSERNOTES_INTERFACE;

#endif

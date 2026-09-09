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

typedef struct _USERNOTES_PROCESS_NOTES
{
    BOOLEAN MatchedCommandLine;
    BOOLEAN Collapse;
    BOOLEAN Boost;
    BOOLEAN Efficiency;

    PPH_STRING Comment;
    ULONG PriorityClass;
    ULONG IoPriorityPlusOne;
    ULONG PagePriorityPlusOne;
    ULONG BackColor;
    KAFFINITY AffinityMask;
} USERNOTES_PROCESS_NOTES, *PUSERNOTES_PROCESS_NOTES;

_Success_(return)
typedef BOOLEAN (NTAPI* PUSERNOTES_GET_PROCESS_NOTES)(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _Out_ PUSERNOTES_PROCESS_NOTES Notes
    );

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

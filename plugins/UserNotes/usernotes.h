/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2016-2022
 *
 */

#ifndef _USERNOTES_H_
#define _USERNOTES_H_

#include <phdk.h>
#include <phappresource.h>
#include <settings.h>

#include "db.h"
#include "resource.h"

#define INTENT_PROCESS_COMMENT 0x1
#define INTENT_PROCESS_PRIORITY_CLASS 0x2
#define INTENT_PROCESS_IO_PRIORITY 0x4
#define INTENT_PROCESS_HIGHLIGHT 0x8
#define INTENT_PROCESS_COLLAPSE 0x10
#define INTENT_PROCESS_AFFINITY 0x20
#define INTENT_PROCESS_PAGEPRIORITY 0x40
#define INTENT_PROCESS_BOOST 0x80

typedef enum _USERNOTES_COMMAND_ID
{
    PROCESS_PRIORITY_SAVE_ID = 1,
    PROCESS_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID,
    PROCESS_PRIORITY_SAVE_IFEO,
    PROCESS_IO_PRIORITY_SAVE_ID,
    PROCESS_IO_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID,
    PROCESS_IO_PRIORITY_SAVE_IFEO,
    PROCESS_HIGHLIGHT_ID,
    PROCESS_COLLAPSE_ID,
    PROCESS_AFFINITY_SAVE_ID,
    PROCESS_AFFINITY_SAVE_FOR_THIS_COMMAND_LINE_ID,
    PROCESS_PAGE_PRIORITY_SAVE_ID,
    PROCESS_PAGE_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID,
    PROCESS_PAGE_PRIORITY_SAVE_IFEO,
    PROCESS_BOOST_PRIORITY_ID,
    PROCESS_BOOST_PRIORITY_SAVE_ID,
    PROCESS_BOOST_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID,
    FILE_PRIORITY_SAVE_IFEO,
    FILE_IO_PRIORITY_SAVE_IFEO,
    FILE_PAGE_PRIORITY_SAVE_IFEO,
} USERNOTES_COMMAND_ID;

#define COMMENT_COLUMN_ID 1

typedef struct _PROCESS_EXTENSION
{
    LIST_ENTRY ListEntry;
    PPH_PROCESS_ITEM ProcessItem;
    PPH_STRING Comment;
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN Valid : 1;
            BOOLEAN SkipAffinity : 1;
            BOOLEAN SkipPriority : 1;
            BOOLEAN SkipPagePriority : 1;
            BOOLEAN SkipIoPriority : 1;
            BOOLEAN SkipBoostPriority : 1;
            BOOLEAN Spare : 2;
        };
    };
} PROCESS_EXTENSION, *PPROCESS_EXTENSION;

typedef struct _PROCESS_COMMENT_PAGE_CONTEXT
{
    HWND CommentHandle;
    HWND RevertHandle;
    HWND MatchCommandlineHandle;
    PPH_STRING OriginalComment;
} PROCESS_COMMENT_PAGE_CONTEXT, *PPROCESS_COMMENT_PAGE_CONTEXT;

typedef struct _SERVICE_EXTENSION
{
    LIST_ENTRY ListEntry;
    BOOLEAN Valid;
    PPH_STRING Comment;
} SERVICE_EXTENSION, *PSERVICE_EXTENSION;

typedef struct _SERVICE_COMMENT_PAGE_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    PH_LAYOUT_MANAGER LayoutManager;
} SERVICE_COMMENT_PAGE_CONTEXT, *PSERVICE_COMMENT_PAGE_CONTEXT;

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK ProcessCommentPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK ServiceCommentPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

UINT_PTR CALLBACK ColorDlgHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif

/*
 * Process Hacker User Notes -
 *   UserNotes Header
 *
 * Copyright (C) 2011-2015 wj32
 * Copyright (C) 2016 dmex
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

#ifndef _USERNOTES_H_
#define _USERNOTES_H_

#include <phdk.h>
#include <phappresource.h>
#include <mxml.h>

#include <windowsx.h>
#include <shlobj.h>
#include <uxtheme.h>

#include "db.h"
#include "resource.h"

#define INTENT_PROCESS_COMMENT 0x1
#define INTENT_PROCESS_PRIORITY_CLASS 0x2
#define INTENT_PROCESS_IO_PRIORITY 0x4
#define INTENT_PROCESS_HIGHLIGHT 0x8
#define INTENT_PROCESS_COLLAPSE 0x10
#define INTENT_PROCESS_AFFINITY 0x20

typedef enum _USERNOTES_COMMAND_ID
{
    PROCESS_PRIORITY_SAVE_ID = 1,
    PROCESS_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID,
    PROCESS_IO_PRIORITY_SAVE_ID,
    PROCESS_IO_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID,
    PROCESS_HIGHLIGHT_ID,
    PROCESS_COLLAPSE_ID,
    PROCESS_AFFINITY_SAVE_ID,
    PROCESS_AFFINITY_SAVE_FOR_THIS_COMMAND_LINE_ID,
} USERNOTES_COMMAND_ID;

#define COMMENT_COLUMN_ID 1

typedef struct _PROCESS_EXTENSION
{
    LIST_ENTRY ListEntry;
    PPH_PROCESS_ITEM ProcessItem;
    BOOLEAN Valid;
    PPH_STRING Comment;
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
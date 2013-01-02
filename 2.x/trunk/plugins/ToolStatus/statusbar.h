/*
 * Process Hacker ToolStatus -
 *   toolstatus header
 *
 * Copyright (C) 2010-2012 wj32
 * Copyright (C) 2011-2012 dmex
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

#ifndef TOOLSTATUSBAR_H
#define TOOLSTATUSBAR_H

extern HWND StatusBarHandle;
extern RECT StatusBarRect;
extern ULONG StatusMask;
extern ULONG ProcessesUpdatedCount;
extern ULONG StatusBarMaxWidths[STATUS_COUNT];

VOID StatusBarCreate(
    __in HWND ParentHandle
    );
VOID UpdateStatusBar(
    VOID
    );
VOID ShowStatusMenu(
    __in PPOINT Point
    );
VOID StatusBarDestroy(
    VOID
    );
#endif
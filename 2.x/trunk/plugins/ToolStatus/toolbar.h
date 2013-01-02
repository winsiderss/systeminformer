/*
 * Process Hacker ToolStatus -
 *   toolstatus header
 *
 * Copyright (C) 2011-2013 dmex
 * Copyright (C) 2010-2012 wj32
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

#ifndef TOOLBAR_H
#define TOOLBAR_H

extern HWND ToolBarHandle;
extern HIMAGELIST ToolBarImageList;
extern HWND TextboxHandle;
extern HFONT TextboxFontHandle;

VOID ToolBarCreate(
    __in HWND ParentHandle
    );

VOID ToolBarDestroy(
    VOID
    );

VOID ToolbarCreateSearch(
    __in HWND ParentHandle
    );

VOID ToolBarCreateImageList(
    __in HWND WindowHandle
    );

VOID ToolbarAddMenuItems(
    __in HWND WindowHandle
    );

#endif
/*
 * Running Object Table Plugin
 *
 * Copyright (C) 2012 dmex
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

#ifndef _ROT_H_
#define _ROT_H_

#define ROT_TABLE_MENUITEM 1000
#define SETTING_PREFIX L"dmex.RunningObjectTable"
#define SETTING_NAME_WINDOWS_WINDOW_POSITION (SETTING_PREFIX L".WindowsWindowPosition")
#define SETTING_NAME_WINDOWS_WINDOW_SIZE (SETTING_PREFIX L".WindowsWindowSize")

#define CINTERFACE
#define COBJMACROS
#include "phdk.h"
#include "phappresource.h"
#include "resource.h"

typedef struct _ROT_WINDOW_CONTEXT
{
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} ROT_WINDOW_CONTEXT, *PROT_WINDOW_CONTEXT;

#endif _ROT_H_
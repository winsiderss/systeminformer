/*
 * Process Hacker Extra Plugins -
 *   Taskbar Extensions
 *
 * Copyright (C) 2015 dmex
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

#ifndef _TB_EXT_H_
#define _TB_EXT_H_

#define PLUGIN_NAME L"dmex.TaskbarExtPlugin"
#define SETTING_NAME_TASKBAR_ICON_TYPE (PLUGIN_NAME L".IconType")

#define CINTERFACE
#define COBJMACROS
#include "phdk.h"
#include "phappresource.h"
#include "resource.h"

#include <Shobjidl.h>

typedef enum _TASKBAR_ICON
{
    TASKBAR_ICON_NONE,
    TASKBAR_ICON_CPU_HISTORY,
    TASKBAR_ICON_IO_HISTORY,
    TASKBAR_ICON_COMMIT_HISTORY,
    TASKBAR_ICON_PHYSICAL_HISTORY,
    TASKBAR_ICON_CPU_USAGE,
} TASKBAR_ICON;

extern PPH_PLUGIN PluginInstance;
extern TASKBAR_ICON TaskbarIconType;

VOID ShowOptionsDialog(
    _In_ HWND ParentHandle
    );

HICON PhGetBlackIcon(
    VOID
    );

HICON PhUpdateIconCpuHistory(
    _In_ PH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

HICON PhUpdateIconIoHistory(
    _In_ PH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

HICON PhUpdateIconCommitHistory(
    _In_ PH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

HICON PhUpdateIconPhysicalHistory(
    _In_ PH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

HICON PhUpdateIconCpuUsage(
    _In_ PH_PLUGIN_SYSTEM_STATISTICS Statistics
    );

#endif _MAIN_H_
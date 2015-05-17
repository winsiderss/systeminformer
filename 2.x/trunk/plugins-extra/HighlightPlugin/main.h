/*
 * Process Hacker Extra Plugins -
 *   Highlight Plugin
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

#ifndef _HIGHLIGHT_H_
#define _HIGHLIGHT_H_

#define SETTING_PREFIX L"dmex.HighlightPlugin"
#define SETTING_NAME_COLOR_LIST (SETTING_PREFIX L".ColorProcessList")
#define SETTING_NAME_CUSTOM_COLOR_LIST (SETTING_PREFIX L".ColorCustomList")

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include <colmgr.h>

#include "resource.h"

typedef enum _PLUGIN_MENU_ITEM_ID
{
    PLUGIN_MENU_DEFAULT_ITEM,
    PLUGIN_MENU_REMOVE_BK_ITEM,
    PLUGIN_MENU_ADD_BK_ITEM,
    //PLUGIN_MENU_REMOVE_FG_ITEM,
    //PLUGIN_MENU_ADD_FG_ITEM,
} PLUGIN_MENU_ITEM_ID;

typedef struct _ITEM_COLOR
{
    COLORREF BackColor;
    //COLORREF ForeColor;
    PPH_STRING ProcessName;
} ITEM_COLOR, *PITEM_COLOR;

#endif
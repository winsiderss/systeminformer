/*
 * Process Hacker Plugins - 
 *   CommonUtil Plugin
 *
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
 *
 */

#ifndef _WCT_H_
#define _WCT_H_

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include <mxml.h>
#include <verify.h>
#include <workqueue.h>
#include <shlobj.h>
#include <Shellapi.h>
#include <psapi.h>
#include <winhttp.h>
#include <windowsx.h>
#include <uxtheme.h>
#include "resource.h"
#include "CommonUtil.h"

#define PLUGIN_NAME L"ProcessHacker.CommonUtil"

extern PPH_PLUGIN PluginInstance;

HICON BitmapToIcon(
    _In_ HBITMAP BitmapHandle,
    _In_ INT Width,
    _In_ INT Height
    );

PVOID UtilCreateJsonArray(
    VOID
    );

PVOID UtilCreateJsonParser(
    _In_ PSTR JsonString
    );

VOID UtilCleanupJsonParser(
    _In_ PVOID Object
    );

PSTR UtilGetJsonValueAsString(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

INT64 UtilGetJsonValueAsUlong(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

VOID UtilAddJsonArray(
    _In_ PVOID jsonArray,
    _In_ PVOID jsonEntry
    );

PVOID UtilCreateJsonObject(
    VOID
    );

PVOID UtilGetJsonObject(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

INT UtilGetJsonObjectLength(
    _In_ PVOID Object
    );

BOOL UtilGetJsonObjectBool(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

VOID UtilJsonAddObject(
    _In_ PVOID Object,
    _In_ PSTR Key,
    _In_ PSTR Value
    );

PSTR UtilGetJsonArrayString(
    _In_ PVOID Object
    );

INT64 UtilGetJsonArrayUlong(
    _In_ PVOID Object,
    _In_ INT Index
    );

INT UtilGetArrayLength(
    _In_ PVOID Object
    );

PVOID UtilGetObjectArrayIndex(
    _In_ PVOID Object,
    _In_ INT Index
    );

PPH_LIST UtilGetObjectArrayList(
    _In_ PVOID Object
    );

#endif
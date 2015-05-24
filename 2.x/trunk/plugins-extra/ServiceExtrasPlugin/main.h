/*
 * Process Hacker Extra Plugins -
 *   Service Extras Plugin
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

#define SETTING_PREFIX L"dmex.ServiceExtrasPlugin"

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include <colmgr.h>

#include "resource.h"

#define SERVICE_SID_COLUMN_ID 1

extern HWND ServiceTreeNewHandle;
extern PPH_PLUGIN PluginInstance;

typedef struct _SERVICE_EXTRA_CONTEXT
{
    HWND ServiceProtectCombo;
    HWND ServiceSidCombo;
    HWND ServiceApplyButton;
    PH_LAYOUT_MANAGER LayoutManager;
} SERVICE_EXTRA_CONTEXT, *PSERVICE_EXTRA_CONTEXT;

typedef struct _SERVICE_EXTENSION
{
    LIST_ENTRY ListEntry;
    BOOLEAN Valid;
    PPH_STRING ServiceSid;
} SERVICE_EXTENSION, *PSERVICE_EXTENSION;

typedef NTSTATUS (NTAPI* _RtlCreateServiceSid)(
    _In_ PUNICODE_STRING ServiceName,
    _Out_writes_bytes_(*ServiceSidLength) PSID ServiceSid,
    _Inout_ PULONG ServiceSidLength
    );
extern _RtlCreateServiceSid RtlCreateServiceSid_I;

VOID UpdateServiceSid(
    _In_ PPH_SERVICE_NODE Node,
    _In_ PSERVICE_EXTENSION Extension
    );

INT_PTR CALLBACK ServiceExtraDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
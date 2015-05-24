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

typedef struct _SERVICE_EXTRA_CONTEXT
{
    HWND ServiceProtectCombo;
    HWND ServiceSidCombo;
    HWND ServiceApplyButton;
    PH_LAYOUT_MANAGER LayoutManager;
} SERVICE_EXTRA_CONTEXT, *PSERVICE_EXTRA_CONTEXT;

INT_PTR CALLBACK ServiceExtraDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
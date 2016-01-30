/*
 * Process Hacker Online Checks -
 *   Main Headers
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2012-2016 dmex
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

#ifndef ONLNCHK_H
#define ONLNCHK_H

#pragma comment(lib, "Winhttp.lib")

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <windowsx.h>
#include <winhttp.h>

#include "sha256.h"
#include "resource.h"

#define PLUGIN_NAME L"ProcessHacker.OnlineChecks"

#define HASH_SHA1 1
#define HASH_SHA256 2

#define UM_EXISTS (WM_USER + 1)
#define UM_LAUNCH (WM_USER + 2)
#define UM_ERROR (WM_USER + 3)

#define Control_Visible(hWnd, visible) \
    ShowWindow(hWnd, visible ? SW_SHOW : SW_HIDE);

typedef enum _PH_UPLOAD_SERVICE_STATE
{
    PhUploadServiceDefault = 0,
    PhUploadServiceChecking,
    PhUploadServiceViewReport,
    PhUploadServiceUploading,
    PhUploadServiceLaunching,
    PhUploadServiceMaximum
} PH_UPLOAD_SERVICE_STATE;

typedef struct _SERVICE_INFO
{
    ULONG Id;
    PWSTR HostName;
    INTERNET_PORT HostPort;
    ULONG HostFlags;
    PWSTR UploadObjectName;
    PWSTR FileNameFieldName;
} SERVICE_INFO, *PSERVICE_INFO;

typedef struct _UPLOAD_CONTEXT
{
    ULONG Service;
    HWND DialogHandle;
    HWND MessageHandle;
    HWND StatusHandle;
    HWND ProgressHandle;
    HFONT MessageFont;
    HINTERNET HttpHandle;

    ULONG ErrorCode;
    ULONG TotalFileLength;

    PH_UPLOAD_SERVICE_STATE UploadServiceState;

    PPH_STRING FileName;
    PPH_STRING BaseFileName;
    PPH_STRING WindowFileName;
    PPH_STRING ObjectName;
    PPH_STRING LaunchCommand;
} UPLOAD_CONTEXT, *PUPLOAD_CONTEXT;

// main
extern PPH_PLUGIN PluginInstance;

// upload
#define UPLOAD_SERVICE_VIRUSTOTAL 101
#define UPLOAD_SERVICE_JOTTI 102
#define UPLOAD_SERVICE_CIMA 103

VOID UploadToOnlineService(
    _In_ PPH_STRING FileName,
    _In_ ULONG Service
    );

#endif

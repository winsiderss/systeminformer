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

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include <mxml.h>
#include <commonutil.h>
#include <workqueue.h>
#include <shlobj.h>
#include <windowsx.h>
#include <winhttp.h>

#include "resource.h"
#include "db.h"

#define PLUGIN_NAME L"ProcessHacker.OnlineChecks"
#define SETTING_NAME_VIRUSTOTAL_SCAN_ENABLED (PLUGIN_NAME L".EnableVirusTotalScanning")

#ifdef VIRUSTOTAL_API
#include "virustotal.h"
#else
#define VIRUSTOTAL_URLPATH L""
#define VIRUSTOTAL_APIKEY L""
#endif

#define UM_EXISTS (WM_USER + 1)
#define UM_LAUNCH (WM_USER + 2)
#define UM_ERROR (WM_USER + 3)

#define Control_Visible(hWnd, visible) \
    ShowWindow(hWnd, visible ? SW_SHOW : SW_HIDE);

extern PPH_PLUGIN PluginInstance;

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

// upload
#define ENABLE_SERVICE_VIRUSTOTAL 100
#define UPLOAD_SERVICE_VIRUSTOTAL 101
#define UPLOAD_SERVICE_JOTTI 102

VOID UploadToOnlineService(
    _In_ PPH_STRING FileName,
    _In_ ULONG Service
    );

typedef struct _PROCESS_EXTENSION
{
    LIST_ENTRY ListEntry;
        
    BOOLEAN Flags;
    struct
    {
        BOOLEAN Stage1 : 1;
        BOOLEAN ResultValid : 1;
        BOOLEAN Spare : 6;
    };

    INT64 Retries;
    INT64 Positives;
    PPH_STRING VirusTotalResult;
    PPH_PROCESS_ITEM ProcessItem;
} PROCESS_EXTENSION, *PPROCESS_EXTENSION;

typedef enum _NETWORK_COLUMN_ID
{
    NETWORK_COLUMN_ID_VIUSTOTAL = 1,
    NETWORK_COLUMN_ID_LOCAL_SERVICE = 2,
} NETWORK_COLUMN_ID;

NTSTATUS HashFileAndResetPosition(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER FileSize,
    _In_ PH_HASH_ALGORITHM Algorithm,
    _Out_ PVOID Hash
    );

typedef struct _VIRUSTOTAL_FILE_HASH_ENTRY
{
    BOOLEAN Flags;
    struct
    {
        BOOLEAN Stage1 : 1;
        BOOLEAN Processing : 1;
        BOOLEAN Processed : 1;
        BOOLEAN Found : 1;
        BOOLEAN Spare : 5;
    };

    PPROCESS_EXTENSION Extension;

    INT64 Positives;
    PPH_STRING FileName;
    PPH_STRING FileHash;
    PPH_BYTES FileNameAnsi;
    PPH_BYTES FileHashAnsi;
    PPH_BYTES CreationTime;
    PPH_STRING FileResult;
} VIRUSTOTAL_FILE_HASH_ENTRY, *PVIRUSTOTAL_FILE_HASH_ENTRY;

typedef struct _VIRUSTOTAL_API_RESULT
{
    BOOLEAN Found;
    INT64 Positives;
    INT64 Total;
    PPH_STRING Permalink;
    PPH_STRING FileHash;
    PPH_STRING DetectionRatio;
} VIRUSTOTAL_API_RESULT, *PVIRUSTOTAL_API_RESULT;

extern PPH_LIST VirusTotalList;
extern BOOLEAN VirusTotalScanningEnabled;

PVIRUSTOTAL_FILE_HASH_ENTRY VirusTotalAddCacheResult(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPROCESS_EXTENSION Extension
    );

PVIRUSTOTAL_FILE_HASH_ENTRY VirusTotalGetCachedResult(
    _In_ PPH_STRING FileName
    );

VOID InitializeVirusTotalProcessMonitor(
    VOID
    );

VOID CleanupVirusTotalProcessMonitor(
    VOID
    );

#endif

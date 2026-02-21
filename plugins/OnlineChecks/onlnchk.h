/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2012-2016
 *
 */

#ifndef ONLNCHK_H
#define ONLNCHK_H

#include <phdk.h>
#include <phappresource.h>
#include <settings.h>
#include <http.h>
#include <json.h>
#include <workqueue.h>
#include <mapimg.h>

#include "resource.h"

#define PLUGIN_NAME L"OnlineChecks"
#define SETTING_NAME_AUTO_SCAN_ENABLED (PLUGIN_NAME L".EnableAutoScanning")
#define SETTING_NAME_VIRUSTOTAL_DEFAULT_ACTION (PLUGIN_NAME L".VirusTotalDefautAction")
#define SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT (PLUGIN_NAME L".VirusTotalDefautPAT")
#define SETTING_NAME_HYBRIDANAL_DEFAULT_PAT (PLUGIN_NAME L".HybridAnalysisDefautPAT")
#define SETTING_NAME_FILESCAN_DEFAULT_PAT (PLUGIN_NAME L".FileScanDefautPAT")

#define UM_UPLOAD (WM_APP + 1)
#define UM_EXISTS (WM_APP + 2)
#define UM_LAUNCH (WM_APP + 3)
#define UM_ERROR (WM_APP + 4)
#define UM_SHOWDIALOG (WM_APP + 5)

extern PPH_PLUGIN PluginInstance;

typedef struct _SERVICE_INFO
{
    ULONG Id;
    PCWSTR HostName;
    PCWSTR UploadObjectName;
    PCWSTR FileNameFieldName;
} SERVICE_INFO, *PSERVICE_INFO;

typedef enum _SCAN_TYPE
{
    SCAN_TYPE_VIRUSTOTAL = 0,
    SCAN_TYPE_HYBRIDANALYSIS = 1,
    SCAN_TYPE_MAX = 2,
} SCAN_TYPE;

typedef struct _SCAN_ITEM SCAN_ITEM;
typedef struct _SCAN_ITEM* PSCAN_ITEM;
typedef struct _SCAN_HASH SCAN_HASH;
typedef struct _SCAN_HASH* PSCAN_HASH;

typedef struct _SCAN_CONTEXT
{
    PPH_STRING FileName;
    PSCAN_HASH FileHash;
    PSCAN_ITEM ScanItems[SCAN_TYPE_MAX];
} SCAN_CONTEXT, *PSCAN_CONTEXT;

typedef enum _SCAN_EXTENSION_TYPE
{
    SCAN_EXTENSION_PROCESS,
    SCAN_EXTENSION_MODULE,
    SCAN_EXTENSION_SERVICE,
} SCAN_EXTENSION_TYPE;

typedef struct _SCAN_EXTENSION
{
    LIST_ENTRY ListEntry;
    SCAN_EXTENSION_TYPE Type;

    union
    {
        PPH_PROCESS_ITEM ProcessItem;
        PPH_MODULE_ITEM ModuleItem;
        PPH_SERVICE_ITEM ServiceItem;
    };

    PPH_STRING FileName;
    SCAN_CONTEXT ScanContext;
    PPH_STRING ScanResults[SCAN_TYPE_MAX];
} SCAN_EXTENSION, *PSCAN_EXTENSION;

typedef struct _UPLOAD_CONTEXT
{
    BOOLEAN VtApiUpload;
    BOOLEAN FileExists;
    BOOLEAN Cancel;
    BOOLEAN ProgressMarquee;
    BOOLEAN ProgressTimer;

    ULONG Service;
    ULONG ErrorCode;
    ULONG TotalFileLength;

    HWND DialogHandle;
    WNDPROC DialogWindowProc;

    //HANDLE TaskbarListClass;

    PPH_STRING HybridPat;
    PPH_STRING TotalPat;
    PPH_STRING FileScanPat;

    PPH_STRING FileSize;
    PPH_STRING ErrorString;
    PPH_STRING FileName;
    PPH_STRING BaseFileName;
    PPH_STRING LaunchCommand;

    PPH_STRING Detected;
    PPH_STRING FirstAnalysisDate;
    PPH_STRING LastAnalysisDate;
    PPH_STRING LastAnalysisUrl;

    PPH_STRING FileHash;
    PPH_STRING FileUpload;

    LONG64 ProgressTotal;
    LONG64 ProgressUploaded;
    LONG64 ProgressBitsPerSecond;
} UPLOAD_CONTEXT, *PUPLOAD_CONTEXT;

#ifdef FORCE_FAST_STATUS_TIMER
#define SETTING_NAME_STATUS_TIMER_INTERVAL USER_TIMER_MINIMUM
#else
#define SETTING_NAME_STATUS_TIMER_INTERVAL 20
#endif

// options.c

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UploadFileThreadStart(
    _In_ PVOID Parameter
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UploadCheckThreadStart(
    _In_ PVOID Parameter
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UploadRecheckThreadStart(
    _In_ PVOID Parameter
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS ViewReportThreadStart(
    _In_ PVOID Parameter
    );

VOID ShowFileUploadDialog(
    _In_ PUPLOAD_CONTEXT Context
    );

VOID ShowFileFoundDialog(
    _In_ PUPLOAD_CONTEXT Context
    );

VOID ShowFileUploadProgressDialog(
    _In_ PUPLOAD_CONTEXT Context
    );

VOID ShowVirusTotalReScanProgressDialog(
    _In_ PUPLOAD_CONTEXT Context
    );

VOID ShowVirusTotalViewReportProgressDialog(
    _In_ PUPLOAD_CONTEXT Context
    );

VOID VirusTotalShowErrorDialog(
    _In_ PUPLOAD_CONTEXT Context
    );

// upload
#define ENABLE_SERVICE_HYBRIDANALYSIS 100
#define MENUITEM_HYBRIDANALYSIS_QUEUE 101
#define MENUITEM_HYBRIDANALYSIS_UPLOAD 102
#define MENUITEM_HYBRIDANALYSIS_UPLOAD_SERVICE 103
#define MENUITEM_HYBRIDANALYSIS_UPLOAD_FILE 104

#define MENUITEM_VIRUSTOTAL_QUEUE 111
#define MENUITEM_VIRUSTOTAL_UPLOAD 112
#define MENUITEM_VIRUSTOTAL_UPLOAD_SERVICE 113
#define MENUITEM_VIRUSTOTAL_UPLOAD_FILE 114

#define MENUITEM_JOTTI_UPLOAD 120
#define MENUITEM_JOTTI_UPLOAD_SERVICE 121
#define MENUITEM_JOTTI_UPLOAD_FILE 122

#define MENUITEM_FILESCANIO_UPLOAD 130
#define MENUITEM_FILESCANIO_UPLOAD_SERVICE 131
#define MENUITEM_FILESCANIO_UPLOAD_FILE 132

VOID UploadToOnlineService(
    _In_ PPH_STRING FileName,
    _In_ ULONG Service
    );

VOID UploadServiceToOnlineService(
    _In_opt_ HWND WindowHandle,
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ ULONG Service
    );

typedef enum _COLUMN_ID
{
    COLUMN_ID_VIUSTOTAL_PROCESS = 1,
    COLUMN_ID_VIUSTOTAL_MODULE = 2,
    COLUMN_ID_VIUSTOTAL_SERVICE = 3,
    COLUMN_ID_HYBRIDANALYSIS_PROCESS = 4,
    COLUMN_ID_HYBRIDANALYSIS_MODULE = 5,
    COLUMN_ID_HYBRIDANALYSIS_SERVICE = 6,
} COLUMN_ID;

NTSTATUS HashFileAndResetPosition(
    _In_ HANDLE FileHandle,
    _In_ PLARGE_INTEGER FileSize,
    _In_ PH_HASH_ALGORITHM Algorithm,
    _Out_ PPH_STRING *HashString
    );

typedef struct _VT_SYSINT_FILE_REPORT_RESULT
{
    PPH_STRING FileName;
    PPH_STRING BaseFileName;

    PPH_STRING Total;
    PPH_STRING Positives;
    PPH_STRING Resource;
    PPH_STRING ScanId;
    PPH_STRING Md5;
    PPH_STRING Sha1;
    PPH_STRING Sha256;
    PPH_STRING ScanDate;
    PPH_STRING Permalink;
    PPH_STRING StatusMessage;
    PPH_LIST ScanResults;
} VT_SYSINT_FILE_REPORT_RESULT, *PVT_SYSINT_FILE_REPORT_RESULT;

PPH_STRING VirusTotalDateToTime(
    _In_ ULONG TimeDateStamp
    );

typedef struct _VIRUSTOTAL_API_RESPONSE
{
    INT64 ResponseCode;
    PPH_STRING StatusMessage;
    PPH_STRING PermaLink;
    PPH_STRING ScanId;
} VIRUSTOTAL_API_RESPONSE, *PVIRUSTOTAL_API_RESPONSE;

typedef struct _VIRUSTOTAL_FILE_REPORT
{
    ULONG HttpStatus;
    PPH_STRING ScanId;
    PPH_STRING ScanDate;
    ULONG64 Malicious;
    ULONG64 Undetected;
} VIRUSTOTAL_FILE_REPORT, *PVIRUSTOTAL_FILE_REPORT;

NTSTATUS VirusTotalRequestFileReport(
    _In_ PPH_STRING FileHash,
    _In_ PPH_STRING ApiKey,
    _Out_ PVIRUSTOTAL_FILE_REPORT* FileReport
    );

VOID VirusTotalFreeFileReport(
    _In_ PVIRUSTOTAL_FILE_REPORT FileReport
    );

PVIRUSTOTAL_API_RESPONSE VirusTotalRequestFileReScan(
    _In_ PPH_STRING FileHash,
    _In_ PPH_STRING FilePat
    );

VOID VirusTotalFreeFileReScan(
    _In_ PVIRUSTOTAL_API_RESPONSE FileReScan
    );

typedef struct _HYBRIDANALYSIS_FILE_REPORT
{
    ULONG HttpStatus;
    ULONG64 ThreatScore;
    PPH_STRING Verdict;
    ULONG64 MultiscanResult;
    PPH_STRING VxFamily;
} HYBRIDANALYSIS_FILE_REPORT, *PHYBRIDANALYSIS_FILE_REPORT;

NTSTATUS HybridAnalysisRequestFileReport(
    _In_ PPH_STRING FileHash,
    _In_ PPH_STRING ApiKey,
    _Out_ PHYBRIDANALYSIS_FILE_REPORT* FileReport
    );

VOID HybridAnalysisFreeFileReport(
    _In_ PHYBRIDANALYSIS_FILE_REPORT FileReport
    );

// scan

#define MENUITEM_VIRUSTOTAL_SCAN_PROCESS 200
#define MENUITEM_HYBRIDANALYSIS_SCAN_PROCESS 201
#define MENUITEM_VIRUSTOTAL_SCAN_MODULE 202
#define MENUITEM_HYBRIDANALYSIS_SCAN_MODULE 203
#define MENUITEM_VIRUSTOTAL_SCAN_SERVICE 204
#define MENUITEM_HYBRIDANALYSIS_SCAN_SERVICE 205

BOOLEAN InitializeScanning(
    VOID
    );

VOID CleanupScanning(
    VOID
    );

VOID InitializeScanContext(
    _Out_ PSCAN_CONTEXT Context
    );

#define SCAN_FLAG_RESCAN        0x00000001
#define SCAN_FLAG_LOCAL_ONLY    0x00000002

typedef
_Function_class_(SCAN_COMPLETE_CALLBACK)
VOID NTAPI SCAN_COMPLETE_CALLBACK(
    _In_ SCAN_TYPE Type,
    _In_ PPH_STRING FileName,
    _In_ PSCAN_HASH FileHash,
    _In_ PPH_STRING Result,
    _In_opt_ PVOID Context
    );
typedef SCAN_COMPLETE_CALLBACK* PSCAN_COMPLETE_CALLBACK;

VOID EnqueueScan(
    _In_ PSCAN_CONTEXT Context,
    _In_ SCAN_TYPE Type,
    _In_ PPH_STRING FileName,
    _In_ ULONG Flags,
    _In_opt_ PSCAN_COMPLETE_CALLBACK Callback,
    _In_opt_ PVOID CallbackContext
    );

BOOLEAN ScanHashEqual(
    _In_ PSCAN_HASH Hash1,
    _In_ PSCAN_HASH Hash2
    );

VOID EvaluateScanContext(
    _In_ PLARGE_INTEGER SystemTime,
    _Inout_ PSCAN_CONTEXT Context,
    _In_ PPH_STRING FileName,
    _In_ ULONG Flags
    );

VOID DeleteScanContext(
    _In_ PSCAN_CONTEXT Context
    );

PPH_STRING ReferenceScanResult(
    _In_ PSCAN_CONTEXT Context,
    _In_ SCAN_TYPE Type
    );

#endif

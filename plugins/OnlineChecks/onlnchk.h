#ifndef ONLNCHK_H
#define ONLNCHK_H

#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "Winhttp.lib")

#define CINTERFACE
#define COBJMACROS
#include <windowsx.h>
#include <time.h>
#include <phdk.h>
#include <OleCtl.h>
#include <winhttp.h>
#include <Netlistmgr.h>
#include <ShObjIdl.h>

#include "sha256.h"
#include "resource.h"

#define HASH_SHA1 1
#define HASH_SHA256 2

#define UM_EXISTS (WM_APP + 1)
#define UM_LAUNCH (WM_APP + 2)
#define UM_ERROR (WM_APP + 3)

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
    PPH_STRING FileName;
    PPH_STRING WindowFileName;
    ULONG Service;
    HWND DialogHandle;
    HWND MessageHandle;
    HWND StatusHandle;
    HWND ProgressHandle;
    HFONT MessageFont;
    HINTERNET HttpHandle;  
    
    ULONG ErrorCode;
    PPH_STRING ErrorMessage;
    PPH_STRING ErrorStatusMessage;

    PH_UPLOAD_SERVICE_STATE UploadServiceState;
    HANDLE FileHandle;
    ULONG TotalFileLength;
    PPH_STRING BaseFileName;
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

INT_PTR CALLBACK UploadDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif

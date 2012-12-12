#ifndef ONLNCHK_H
#define ONLNCHK_H

#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "Winhttp.lib")

#define COBJMACROS
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

#define UM_LAUNCH_COMMAND (WM_APP + 1)
#define UM_ERROR (WM_APP + 2)

typedef struct _UPLOAD_CONTEXT
{
    PPH_STRING FileName;
    ULONG Service;
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HANDLE ThreadHandle;

    PPH_STRING LaunchCommand;
    PPH_STRING ErrorMessage;
} UPLOAD_CONTEXT, *PUPLOAD_CONTEXT;

typedef struct _SERVICE_INFO
{
    ULONG Id;
    PWSTR HostName;
    INTERNET_PORT HostPort;
    ULONG HostFlags;
    PWSTR UploadObjectName;
    PWSTR FileNameFieldName;
} SERVICE_INFO, *PSERVICE_INFO;


// main
extern PPH_PLUGIN PluginInstance;

// upload
#define UPLOAD_SERVICE_VIRUSTOTAL 101
#define UPLOAD_SERVICE_JOTTI 102
#define UPLOAD_SERVICE_CIMA 103

VOID UploadToOnlineService(
    __in HWND hWnd,
    __in PPH_STRING FileName,
    __in ULONG Service
    );

INT_PTR CALLBACK UploadDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#endif

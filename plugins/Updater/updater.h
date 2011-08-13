#ifndef NETTOOLS_H
#define NETTOOLS_H
#ifndef UNICODE
#define UNICODE
#endif

#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "Advapi32.lib")

#include "phdk.h"
#include "resource.h"
#include <stdio.h>
#include <windows.h>
#include <wininet.h>
#include "Urlmon.h"
#include "mxml.h"
#include <Wincrypt.h>
#include <stdio.h>
#include <windows.h>
#include <Wincrypt.h>

#define MD5LEN  16

#define BUFFER_LEN 512
#define DEFAULT_TIMEOUT 2 * 60 * 1000 // Two minutes
#define UPDATE_MENUITEM 1

#ifndef MAIN_PRIVATE
extern PPH_PLUGIN PluginInstance;
#endif

static HANDLE dlFile;
static PPH_HASH_CONTEXT hashCtx;
static BOOL Install = FALSE;
static HINTERNET initialize, connection, file;
static PPH_STRING remoteVersion;
static PPH_STRING localFilePath;

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

VOID DisposeHandles();
BOOL PhInstalledUsingSetup();

INT VersionParser(char* version1, char* version2);

VOID LogEvent(__in __format_string PWSTR Format,
    ...);

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

INT_PTR CALLBACK NetworkOutputDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID InitializeConnection(BOOL useCache, PCWSTR host, PCWSTR path);

#endif
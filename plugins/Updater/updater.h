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
#include "../../ProcessHacker/mxml/mxml.h"
#include <Wincrypt.h>

#define BUFFER_LEN 512
#define DEFAULT_TIMEOUT 2 * 60 * 1000 // Two minutes
#define UPDATE_MENUITEM 1

static BOOL Install = FALSE;
static HINTERNET initialize, connection, file;
static PPH_STRING remoteVersion;
static PPH_STRING localFilePath;

#ifndef MAIN_PRIVATE
extern PPH_PLUGIN PluginInstance;
#endif

BOOL PhInstalledUsingSetup();

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;

INT_PTR CALLBACK NetworkOutputDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#endif
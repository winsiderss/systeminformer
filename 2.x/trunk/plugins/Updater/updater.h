#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "Advapi32.lib")

#include "phdk.h"
#include "resource.h"
#include "wininet.h"
#include "mxml.h"

#define BUFFER_LEN 512
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

DWORD InitializeConnection(
	__in BOOL useCache,
	__in PCWSTR host,
	__in PCWSTR path
	);

DWORD CreateTempPath();

VOID wtoc(
	CHAR* Dest, 
	const WCHAR* Source
	);

VOID LogEvent(
	__in PPH_STRING str
	);

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
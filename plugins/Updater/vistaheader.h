#pragma once

#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "Shell32.lib")

#include "phdk.h"
#include "resource.h"
#include "wininet.h"
#include "mxml.h"
#include "windowsx.h"

#include <ShellAPI.h>
#include <ShlObj.h>
#include <stdint.h>

#define TDIF_SIZE_TO_CONTENT 0x1000000

#define SecurityStop UINT16_MAX - 1
#define SecurityInformation UINT16_MAX - 2
#define SecurityShield  UINT16_MAX - 3
#define SecurityShieldBlue UINT16_MAX - 4
#define SecurityWarning UINT16_MAX - 5
#define SecurityError UINT16_MAX - 6
#define SecuritySuccess UINT16_MAX - 7
#define SecurityShieldGray UINT16_MAX - 8
#define ASecurityWarning UINT16_MAX

#define UPDATE_URL L"processhacker.sourceforge.net"
#define UPDATE_FILE L"/update.php"

#define DOWNLOAD_SERVER L"sourceforge.net"
#define DOWNLOAD_PATH L"/projects/processhacker/files/processhacker2/%s/download" /* ?use_mirror=waix" */

#define BUFFER_LEN 512
#define UPDATE_MENUITEM 1

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

typedef struct _UPDATER_XML_DATA
{
    ULONG MinorVersion;
    ULONG MajorVersion;
    PPH_STRING RelDate;
    PPH_STRING Size;
    PPH_STRING Hash;
} UPDATER_XML_DATA, *PUPDATER_XML_DATA;

#pragma region Functions

void VistaStartInitialCheck(void);
void ShowUpdateTaskDialog(void);

BOOL PhInstalledUsingSetup(void);
BOOL ConnectionAvailable(void);

void DisposeConnection(void);
void DisposeStrings(void);
void DisposeFileHandles(void);

BOOL VistaInitializeConnection(
    __in HWND hwndDlg,
    __in PCWSTR host,
    __in PCWSTR path
    );

BOOL ParseVersionString(
    __in PWSTR String,
    __out PULONG MajorVersion,
    __out PULONG MinorVersion
    );
LONG CompareVersions(
    __in ULONG MajorVersion1,
    __in ULONG MinorVersion1,
    __in ULONG MajorVersion2,
    __in ULONG MinorVersion2
    );
BOOL ReadRequestString(
    __in HINTERNET Handle,
    __out PSTR *Data,
    __out_opt PULONG DataLength
    );
BOOL QueryXmlData(
    __in PVOID Buffer,
    __out PUPDATER_XML_DATA XmlData
    );
VOID FreeXmlData(
    __in PUPDATER_XML_DATA XmlData
    );
BOOL InitializeConnection(
    __in PCWSTR host,
    __in PCWSTR path
    );
VOID LogEvent(
    __in PPH_STRING str
    );
HRESULT CALLBACK TaskDlgDownloadPageWndProc(
    __in HWND hwndDlg, 
    __in UINT uMsg, 
    __in WPARAM wParam, 
    __in LPARAM lParam, 
    __in LONG_PTR lpRefData
    );

HRESULT CALLBACK TaskDlgUpdatePageWndProc(
    __in HWND hwndDlg, 
    __in UINT uMsg, 
    __in WPARAM wParam, 
    __in LPARAM lParam, 
    __in LONG_PTR lpRefData
    );

HRESULT CALLBACK TaskDlgWndProc(
    __in HWND hwnd, 
    __in UINT uMsg, 
    __in WPARAM wParam, 
    __in LPARAM lParam, 
    __in LONG_PTR lpRefData
    );

#pragma endregion
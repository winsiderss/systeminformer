#pragma once

#pragma region libs

#pragma comment(lib, "Wininet.lib")

#pragma endregion

#pragma region enums

typedef enum _PH_UPDATER_STATE
{
    Default,
    Downloading,
    Installing
} PH_UPDATER_STATE;

#pragma endregion

#pragma region Includes

#include "phdk.h"
#include "phapppub.h"
#include "resource.h"
#include "wininet.h"
#include "mxml.h"
#include "windowsx.h"

#include <ShlObj.h>
#include <stdint.h>

#pragma endregion

#pragma region Defines

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

#define DOWNLOAD_SERVER L"download.fedoraproject.org"// L"sourceforge.net"
#define DOWNLOAD_PATH L"/pub/alt/spins/linux/releases/15/Spins/i686/Fedora-15-i686-Live-LXDE.iso"// L"/projects/processhacker/files/processhacker2/%s/download" /* ?use_mirror=waix" */

#define BUFFER_LEN 512
#define UPDATE_MENUITEM 1

#define ENABLE_UI WM_APP + 1

#define Updater_SetStatusText(hwndDlg, lpString) \
    SetDlgItemText(hwndDlg, IDC_STATUSTEXT, lpString)

#define Updater_EnableUI(hwndDlg) \
    PostMessage(hwndDlg, ENABLE_UI, 0, 0)

typedef struct _PH_UPDATER_CONTEXT
{
    HWND MainWindowHandle;
    PVOID Parameter;
} PH_UPDATER_CONTEXT, *PPH_UPDATER_CONTEXT;

typedef struct _UPDATER_XML_DATA
{
    ULONG MinorVersion;
    ULONG MajorVersion;
    PPH_STRING RelDate;
    PPH_STRING Size;
    PPH_STRING Hash;
} UPDATER_XML_DATA, *PUPDATER_XML_DATA;

#pragma endregion

#pragma region Instances

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

#pragma endregion

#pragma region Functions
VOID Test();
BOOL VistaInitializeConnection(
    __in PCWSTR host,
    __in PCWSTR path
    );
VOID DisposeConnection(VOID);
VOID DisposeStrings(VOID);
VOID DisposeFileHandles(VOID);

BOOL ConnectionAvailable(VOID);

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

VOID StartInitialCheck(VOID);

VOID ShowUpdateDialog(VOID);

BOOL PhInstalledUsingSetup(VOID);

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

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
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

HRESULT CALLBACK TaskDlgWndProc(
    __in HWND hwnd, 
    __in UINT uMsg, 
    __in WPARAM wParam, 
    __in LPARAM lParam, 
    __in LONG_PTR lpRefData
    );

INT_PTR CALLBACK MainWndProc(
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

#pragma endregion
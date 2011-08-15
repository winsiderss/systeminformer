#pragma region libs

#pragma comment(lib, "Wininet.lib")

#pragma endregion

typedef enum _PH_UPDATER_STATE
{
	Default,
    Downloading,
    Installing
} PH_UPDATER_STATE;

#pragma region Includes

#include "phdk.h"
#include "phapppub.h"
#include "resource.h"
#include "wininet.h"
#include "mxml.h"
#include "windowsx.h"

#pragma endregion

#pragma region Defines

#define BUFFER_LEN 256
#define UPDATE_MENUITEM 1

#define Updater_SetStatusText(hwndDlg, lpString) \
   PostMessage(hwndDlg, WM_APP + 1, (LPARAM)lpString, 0);

typedef struct _PH_UPDATER_CONTEXT
{
    HWND MainWindowHandle;
    PVOID Parameter;
} PH_UPDATER_CONTEXT, *PPH_UPDATER_CONTEXT;

#pragma endregion

#pragma region Globals

extern PPH_PLUGIN PluginInstance;

#pragma endregion

#pragma region Statics

static HANDLE TempFileHandle = NULL;
static HINTERNET NetInitialize = NULL, NetConnection = NULL, NetRequest = NULL;

static PH_STRING *LocalFilePathString = NULL;
static PH_ANSI_STRING *RemoteHashString = NULL;
static PH_UPDATER_STATE PhUpdaterState = Default;
static BOOL EnableCache = TRUE;
static BOOL CheckBetaRelease = FALSE;
static PH_HASH_ALGORITHM HashAlgorithm = Md5HashAlgorithm;

static PH_SETTING_CREATE settings[] =
{
	{ IntegerSettingType, L"ProcessHacker.Updater.EnableCache", L"1" },
	{ IntegerSettingType, L"ProcessHacker.Updater.HashAlgorithm", L"1" },
};	

#pragma endregion

#pragma region Instances

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

#pragma endregion

#pragma region Functions

VOID DisposeConnection();
VOID DisposeStrings();
VOID DisposeFileHandles();

BOOL PhInstalledUsingSetup();

DWORD InitializeConnection(
	__in BOOL useCache,
	__in PCWSTR host,
	__in PCWSTR path
	);

DWORD InitializeFile();

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
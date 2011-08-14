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

#define BUFFER_LEN 512
#define UPDATE_MENUITEM 1

#define Updater_SetStatusText(hwndDlg, lpString) \
   SetDlgItemText(hwndDlg, IDC_STATUS, lpString);

#pragma endregion

#pragma region Globals

extern PPH_PLUGIN PluginInstance;

#pragma endregion

#pragma region Statics

typedef DWORD (WINAPI *_SetTcpEntry)(
    __in DWORD pTcpRow
    );

static HANDLE TempFileHandle = NULL;
static HINTERNET initialize = NULL, connection = NULL, file = NULL;

static PPH_STRING localFilePath = NULL;
static PH_UPDATER_STATE PhUpdaterState = Default;

static BOOL EnableCache = TRUE;

static PH_SETTING_CREATE settings[] =
{
	{ IntegerSettingType, L"ProcessHacker.Updater.EnableCache", L"1" }      
	//{ IntegerPairSettingType, SETTING_NAME_WINDOWS_WINDOW_POSITION, L"100,100" },
	//{ IntegerPairSettingType, SETTING_NAME_WINDOWS_WINDOW_SIZE, L"690,540" }
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
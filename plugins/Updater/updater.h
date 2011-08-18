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

#pragma endregion

#pragma region Defines

#define BUFFER_LEN 512
#define UPDATE_MENUITEM 1

#define Updater_SetStatusText(hwndDlg, lpString) \
	SetDlgItemText(hwndDlg, IDC_STATUSTEXT, lpString)

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
static PPH_ANSI_STRING VersionString = NULL, RemoteHashString = NULL;
static PPH_STRING LocalFilePathString = NULL, ReldateString = NULL, SizeString = NULL, BetaDlString = NULL;

static PH_UPDATER_STATE PhUpdaterState = Default;
static BOOL EnableCache = TRUE;
static BOOL WindowVisible = FALSE;
static PH_HASH_ALGORITHM HashAlgorithm = Md5HashAlgorithm;

extern NTSTATUS SilentWorkerThreadStart(
	__in PVOID Parameter
	);

static PH_SETTING_CREATE settings[] =
{
	{ IntegerSettingType, L"ProcessHacker.Updater.EnableCache", L"1" },
	{ IntegerSettingType, L"ProcessHacker.Updater.HashAlgorithm", L"1" },
	{ IntegerSettingType, L"ProcessHacker.Updater.PromptStart", L"0" },
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

BOOL ConnectionAvailable();
BOOL PhInstalledUsingSetup();
BOOL QueryXmlData(void* buffer);

BOOL InitializeConnection(
	__in PCWSTR host,
	__in PCWSTR path
	);

BOOL InitializeFile();

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
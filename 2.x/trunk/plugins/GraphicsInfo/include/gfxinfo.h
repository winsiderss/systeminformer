#include "resource.h"
#include "nvapi.h"

#include <phdk.h>
#include <graph.h>

#define GFXINFO_MENUITEM 1

#define WM_ET_ETWSYS_ACTIVATE (WM_APP + 150)
#define WM_ET_ETWSYS_UPDATE (WM_APP + 151)
#define WM_ET_ETWSYS_PANEL_UPDATE (WM_APP + 160)

VOID ShowDialog(VOID);

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

static PH_LAYOUT_MANAGER WindowLayoutManager;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI UnloadCallback(
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

INT_PTR CALLBACK MainWndProc(      
	__in HWND hwndDlg,
	__in UINT uMsg,
	__in WPARAM wParam,
	__in LPARAM lParam
	);

INT_PTR CALLBACK EtpEtwSysPanelDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID LogEvent(__in PWSTR str, __in INT status);


VOID NvInit(VOID);
NvPhysicalGpuHandle EnumNvidiaGpuHandles();
VOID GetNvidiaGpuUsages();

// macro by dmex
#define NV_SUCCESS(Status) (((NvAPI_Status)(Status)) == NVAPI_OK)

// dmex - magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34

typedef void* (__cdecl *P_NvAPI_QueryInterface)(UINT offset);
P_NvAPI_QueryInterface NvAPI_QueryInterface;

typedef void* (__cdecl *P_NvApi_CallStart)(UINT, UINT*); // weak
P_NvApi_CallStart NvAPI_CallStart;

typedef int (__cdecl *P_NvApi_CallReturn)(DWORD, DWORD, DWORD, DWORD); // weak
P_NvApi_CallReturn NvAPI_CallReturn;

typedef NvAPI_Status (__cdecl *P_NvAPI_GPU_GetUsages)(NvPhysicalGpuHandle nvGPUHandle, unsigned int *usages);
P_NvAPI_GPU_GetUsages NvAPI_GetUsages;

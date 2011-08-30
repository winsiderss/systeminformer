#include "resource.h"
#include "nvapi.h"

#include <phdk.h>
#include <graph.h>

#include <windowsx.h>

#include "adl_defines.h"
#include "adl_sdk.h"
#include "adl_structures.h"

typedef enum _GFX_TYPE
{
    Default,
    NvidiaGraphics,
    AtiGraphics
} PH_GFX_TYPE;

#define GFXINFO_MENUITEM 1

#define WM_GFX_ACTIVATE (WM_APP + 150)
#define WM_GFX_UPDATE (WM_APP + 151)
#define WM_GFX_PANEL_UPDATE (WM_APP + 160)

#define SETTING_PREFIX L"ProcessHacker.GfxPlugin."
#define SETTING_NAME_GFX_ALWAYS_ON_TOP (SETTING_PREFIX L"GfxAlwaysOnTop")
#define SETTING_NAME_GFX_WINDOW_POSITION (SETTING_PREFIX L"GfxWindowPosition")
#define SETTING_NAME_GFX_WINDOW_SIZE (SETTING_PREFIX L"GfxWindowSize")

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

static PH_LAYOUT_MANAGER WindowLayoutManager;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

VOID ShowDialog(VOID);

VOID InitGfx(VOID);
VOID GetGfxUsages(VOID);
VOID GetDriverVersion(VOID);
VOID GetGfxTemp(VOID);
VOID GetGfxFanSpeed(VOID);
VOID GetGfxClockSpeeds(VOID);

PPH_STRING GetGfxName(VOID);

NvPhysicalGpuHandle EnumNvidiaGpuHandles(VOID);
NvDisplayHandle EnumNvidiaDisplayHandles(VOID);

VOID LogEvent(
    __in PWSTR str, 
    __in NvStatus status
    );

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

#pragma pack(push, 8) // Make sure we have consistent structure packings

#define NV_SUCCESS(Status) (((NvStatus)(Status)) == NVAPI_OK)

// dmex - magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  33
#define MAX_MEMORY_VALUES_PER_GPU 5
#define MAX_COOLER_PER_GPU 20
#define MAX_CLOCKS_PER_GPU 0x120

#define NV_MEMORY_INFO_VER MAKE_NVAPI_VERSION(NV_MEMORY_INFO_V2, 2)
#define NV_USAGES_INFO_VER MAKE_NVAPI_VERSION(NV_USAGES_INFO_V1, 1)
#define NV_COOLER_INFO_VER MAKE_NVAPI_VERSION(NV_COOLER_INFO_V2, 2)
#define NV_CLOCKS_INFO_VER MAKE_NVAPI_VERSION(NV_CLOCKS_INFO_V2, 2)

// rev
typedef struct _NV_MEMORY_INFO
{
    NvU32 Version; // structure version
    NvU32 Values[MAX_MEMORY_VALUES_PER_GPU];
} NV_MEMORY_INFO_V2, *PNV_MEMORY_INFO_V2;

// rev
typedef struct _NV_USAGES_INFO
{
    NvU32 Version; // structure version
    NvU32 Values[NVAPI_MAX_USAGES_PER_GPU];
} NV_USAGES_INFO_V1, *PNV_USAGES_INFO_V1;

typedef struct _NV_COOLER_VALUES_V2
{
    NvU32 Type;
    NvU32 Controller;
    NvU32 DefaultMin;
    NvU32 DefaultMax;
    NvU32 CurrentMin;
    NvU32 CurrentMax;
    NvU32 CurrentLevel;
    NvU32 DefaultPolicy;
    NvU32 CurrentPolicy;
    NvU32 Target;
    NvU32 ControlType;
    NvU32 Active;
} NV_COOLER_VALUES_V2, *PNV_COOLER_VALUES_V2;

// rev
typedef struct _NV_COOLER_INFO
{
    NvU32 Version; // structure version
    NvU32 Count; // number of associated thermal sensors
    NV_COOLER_VALUES_V2 Values[MAX_COOLER_PER_GPU];
} NV_COOLER_INFO_V2, *PNV_COOLER_INFO_V2;

// rev
typedef struct _NV_CLOCKS_INFO
{
    NvU32 Version; // structure version
    NvU32 Values[MAX_CLOCKS_PER_GPU];
} NV_CLOCKS_INFO_V2, *PNV_CLOCKS_INFO_V2;

// rev
typedef PVOID (__cdecl *P_NvAPI_QueryInterface)(UINT);
P_NvAPI_QueryInterface NvAPI_QueryInterface;

// rev
typedef NvStatus (__cdecl *P_NvAPI_GetMemoryInfo)(NvDisplayHandle, PNV_MEMORY_INFO_V2);
P_NvAPI_GetMemoryInfo NvAPI_GetMemoryInfo;

// rev
typedef NvStatus (__cdecl *P_NvAPI_GPU_GetUsages)(NvPhysicalGpuHandle, PNV_USAGES_INFO_V1);
P_NvAPI_GPU_GetUsages NvAPI_GetUsages;

// rev
typedef NvStatus (__cdecl *P_NvAPI_GPU_GetCoolerSettings)(NvPhysicalGpuHandle, NvU32 sensorIndex, PNV_COOLER_INFO_V2);
P_NvAPI_GPU_GetCoolerSettings NvAPI_GetCoolerSettings;

// rev
typedef NvStatus (__cdecl *P_NvAPI_GPU_GetAllClocks)(NvPhysicalGpuHandle, PNV_CLOCKS_INFO_V2);
P_NvAPI_GPU_GetAllClocks NvAPI_GetAllClocks;

#pragma pack(pop)

//typedef int (*ADL_MAIN_CONTROL_CREATE )(ADL_MAIN_MALLOC_CALLBACK, int);
//ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create;

//typedef int (*P_Adl_OVERDRIVE_CURRENTACTIVITY_Get)(int iAdapterIndex,  ADLPMActivity*);
//P_Adl_OVERDRIVE_CURRENTACTIVITY_Get Adl_GetCurrentActivity;

//typedef int (*ADL_MAIN_CONTROL_DESTROY )();
//typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int*);
//typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo, int );
//typedef int (*ADL_DISPLAY_DISPLAYINFO_GET)(int, int*, ADLDisplayInfo**, int);

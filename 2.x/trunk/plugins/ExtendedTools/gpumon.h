#ifndef GPUMON_H
#define GPUMON_H

// setupapi definitions

typedef _Check_return_ HDEVINFO (WINAPI *_SetupDiGetClassDevsW)(
    _In_opt_ CONST GUID *ClassGuid,
    _In_opt_ PCWSTR Enumerator,
    _In_opt_ HWND hwndParent,
    _In_ DWORD Flags
    );

typedef BOOL (WINAPI *_SetupDiDestroyDeviceInfoList)(
    _In_ HDEVINFO DeviceInfoSet
    );

typedef BOOL (WINAPI *_SetupDiEnumDeviceInterfaces)(
    _In_ HDEVINFO DeviceInfoSet,
    _In_opt_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ CONST GUID *InterfaceClassGuid,
    _In_ DWORD MemberIndex,
    _Out_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
    );

typedef BOOL (WINAPI *_SetupDiGetDeviceInterfaceDetailW)(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _Out_writes_bytes_opt_(DeviceInterfaceDetailDataSize) PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,
    _In_ DWORD DeviceInterfaceDetailDataSize,
    _Out_opt_ PDWORD RequiredSize,
    _Out_opt_ PSP_DEVINFO_DATA DeviceInfoData
    );

typedef BOOL (WINAPI *_SetupDiGetDeviceRegistryPropertyW)(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ DWORD Property,
    _Out_opt_ PDWORD PropertyRegDataType,
    _Out_opt_ PBYTE PropertyBuffer,
    _In_ DWORD PropertyBufferSize,
    _Out_opt_ PDWORD RequiredSize
    );

// Macros

#define BYTES_NEEDED_FOR_BITS(Bits) ((((Bits) + sizeof(ULONG) * 8 - 1) / 8) & ~(SIZE_T)(sizeof(ULONG) - 1)) // divide round up

// Structures

typedef struct _ETP_GPU_ADAPTER
{
    LUID AdapterLuid;
    PPH_STRING Description;
    ULONG SegmentCount;
    ULONG NodeCount;
    ULONG FirstNodeIndex;

    BOOLEAN HasActivity;

    RTL_BITMAP ApertureBitMap;
    ULONG ApertureBitMapBuffer[1];
} ETP_GPU_ADAPTER, *PETP_GPU_ADAPTER;

// Functions

BOOLEAN EtpInitializeD3DStatistics(
    VOID
    );

PETP_GPU_ADAPTER EtpAllocateGpuAdapter(
    _In_ ULONG NumberOfSegments
    );

PPH_STRING EtpQueryDeviceDescription(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    );

VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

#endif

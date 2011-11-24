#ifndef GPUMON_H
#define GPUMON_H

// setupapi definitions

typedef __checkReturn HDEVINFO (WINAPI *_SetupDiGetClassDevsW)(
    __in_opt CONST GUID *ClassGuid,
    __in_opt PCWSTR Enumerator,
    __in_opt HWND hwndParent,
    __in DWORD Flags
    );

typedef BOOL (WINAPI *_SetupDiDestroyDeviceInfoList)(
    __in HDEVINFO DeviceInfoSet
    );

typedef BOOL (WINAPI *_SetupDiEnumDeviceInterfaces)(
    __in HDEVINFO DeviceInfoSet,
    __in_opt PSP_DEVINFO_DATA DeviceInfoData,
    __in CONST GUID *InterfaceClassGuid,
    __in DWORD MemberIndex,
    __out PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
    );

typedef BOOL (WINAPI *_SetupDiGetDeviceInterfaceDetailW)(
    __in HDEVINFO DeviceInfoSet,
    __in PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    __out_bcount_opt(DeviceInterfaceDetailDataSize) PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,
    __in DWORD DeviceInterfaceDetailDataSize,
    __out_opt PDWORD RequiredSize, 
    __out_opt PSP_DEVINFO_DATA DeviceInfoData
    );

typedef BOOL (WINAPI *_SetupDiGetDeviceRegistryPropertyW)(
    __in HDEVINFO DeviceInfoSet,
    __in PSP_DEVINFO_DATA DeviceInfoData,
    __in DWORD Property,
    __out_opt PDWORD PropertyRegDataType,
    __out_opt PBYTE PropertyBuffer,
    __in DWORD PropertyBufferSize,
    __out_opt PDWORD RequiredSize
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

    RTL_BITMAP ApertureBitMap;
    ULONG ApertureBitMapBuffer[1];
} ETP_GPU_ADAPTER, *PETP_GPU_ADAPTER;

// Functions

BOOLEAN EtpInitializeD3DStatistics(
    VOID
    );

PETP_GPU_ADAPTER EtpAllocateGpuAdapter(
    __in ULONG NumberOfSegments
    );

PPH_STRING EtpQueryDeviceDescription(
    __in HDEVINFO DeviceInfoSet,
    __in PSP_DEVINFO_DATA DeviceInfoData
    );

VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

#endif

#ifndef GPUMON_H
#define GPUMON_H

// Macros

#define BYTES_NEEDED_FOR_BITS(Bits) ((((Bits) + sizeof(ULONG) * 8 - 1) / 8) & ~(SIZE_T)(sizeof(ULONG) - 1)) // divide round up

// Structures

typedef struct _ETP_GPU_ADAPTER
{
    LUID AdapterLuid;
    ULONG SegmentCount;
    ULONG NodeCount;
    ULONG FirstNodeIndex;

    PPH_STRING DeviceInterface;
    PPH_STRING Description;
    PPH_LIST NodeNameList;

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

_Success_(return)
BOOLEAN EtQueryDeviceProperties(
    _In_ PWSTR DeviceInterface,
    _Out_opt_ PPH_STRING *Description,
    _Out_opt_ PPH_STRING *DriverDate,
    _Out_opt_ PPH_STRING *DriverVersion,
    _Out_opt_ PPH_STRING *LocationInfo,
    _Out_opt_ ULONG64 *InstalledMemory
    );

VOID NTAPI EtGpuProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

#endif

/*
 * Direct3D Kernel Mode Thunk (D3DKMT)
 *
 * This file is part of System Informer.
 */

#ifndef _NTD3DKMT_H
#define _NTD3DKMT_H

#include <dxmini.h>
//#include <d3dkmddi.h>
#include <d3dkmthk.h>
#include <devpropdef.h>

DEFINE_DEVPROPKEY(DEVPKEY_Gpu_Luid, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2); // DEVPROP_TYPE_UINT64
DEFINE_DEVPROPKEY(DEVPKEY_Gpu_PhysicalAdapterIndex, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 3); // DEVPROP_TYPE_UINT32
DEFINE_GUID(GUID_COMPUTE_DEVICE_ARRIVAL, 0x1024e4c9, 0x47c9, 0x48d3, 0xb4, 0xa8, 0xf9, 0xdf, 0x78, 0x52, 0x3b, 0x53);

typedef D3DKMT_HANDLE* PD3DKMT_HANDLE;

typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1
{
    ULONG CommitLimit;
    ULONG BytesCommitted;
    ULONG BytesResident;
    D3DKMT_QUERYSTATISTICS_MEMORY Memory;
    ULONG Aperture; // boolean
    ULONGLONG TotalBytesEvictedByPriority[D3DKMT_MaxAllocationPriorityClass];
    ULONG64 SystemMemoryEndAddress;
    struct
    {
        ULONG64 PreservedDuringStandby : 1;
        ULONG64 PreservedDuringHibernate : 1;
        ULONG64 PartiallyPreservedDuringHibernate : 1;
        ULONG64 Reserved : 61;
    } PowerFlags;
    ULONG64 Reserved[7];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1, *PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1;

/**
 * The D3DKMT_GET_PROCESS_LIST structure is used for retrieving a list of process handles using a graphics adapter.
 * \remarks The caller is responsible for closing the returned process handles.
 */
// rev
typedef struct _D3DKMT_GET_PROCESS_LIST
{
    LUID AdapterLuid;          // [in] The locally unique identifier (LUID) for the graphics adapter.
    ULONG DesiredAccess;       // [in] The access rights to request for the process handles. This must be `PROCESS_QUERY_INFORMATION` (0x400).
    ULONG ProcessHandleCount;  // [in, out] On input, specifies the number of handles the `ProcessHandle` member can hold. On output, receives the number of handles returned.
    HANDLE ProcessHandle;      // [out] The first element of an array that receives the process handles.
} D3DKMT_GET_PROCESS_LIST, *PD3DKMT_GET_PROCESS_LIST;

// rev
/**
 * The D3DKMTGetProcessList function retrieves a list of processes that are using a specific graphics adapter.
 *
 * \param[in,out] GetProcessList A pointer to a \ref D3DKMT_GET_PROCESS_LIST structure that contains the processes using the graphics adapter.
 * \return NTSTATUS Successful or errant status.
 */
EXTERN_C
NTSTATUS
NTAPI
D3DKMTGetProcessList(
    _Inout_ PD3DKMT_GET_PROCESS_LIST GetProcessList
    );

// rev
/**
 * The D3DKMT_ENUM_PROCESS_LIST structure is used for retrieving a list of process identifiers using a graphics adapter.
 */
typedef struct _D3DKMT_ENUM_PROCESS_LIST
{
    LUID AdapterLuid;          // [in] The locally unique identifier (LUID) for the graphics adapter.
    PULONG ProcessIdBuffer;    // [out] A pointer to a buffer that receives the list of process identifiers (PIDs).
    SIZE_T ProcessIdCount;     // [in, out] On input, specifies the number of elements the `ProcessIdBuffer` can hold. On output, receives the number of process IDs returned.
} D3DKMT_ENUM_PROCESS_LIST, *PD3DKMT_ENUM_PROCESS_LIST;

// rev
/**
 * The D3DKMTEnumProcesses function provides a list of process IDs (PIDs) rather than handles that are using a specific graphics adapter, 
 * which can be more efficient for monitoring purposes.
 *
 * \param[in,out] EnumProcessList A pointer to a \ref D3DKMT_ENUM_PROCESS_LIST structure that contains the processes using the graphics adapter.
 * \return NTSTATUS Successful or errant status.
 */
EXTERN_C
NTSTATUS
NTAPI
D3DKMTEnumProcesses(
    _Inout_ PD3DKMT_ENUM_PROCESS_LIST EnumProcessList
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDirectGraphicsCall(
    _In_ ULONG InputBufferLength,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _Out_ PULONG ReturnLength
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)

#endif // _NTD3DKMT_H

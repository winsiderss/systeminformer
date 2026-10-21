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

EXTERN_C_START

DEFINE_DEVPROPKEY(DEVPKEY_Gpu_Luid, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2); // DEVPROP_TYPE_UINT64
DEFINE_DEVPROPKEY(DEVPKEY_Gpu_PhysicalAdapterIndex, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 3); // DEVPROP_TYPE_UINT32
// rev
DEFINE_DEVPROPKEY(DEVPKEY_Gpu_IddVirtualMonitorDevice, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 4); // DEVPROP_TYPE_BOOLEAN
DEFINE_DEVPROPKEY(DEVPKEY_Gpu_Parent_Luid, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 6); // DEVPROP_TYPE_INT64

// Device-interface class registered for compute devices, not a PnP setup class.
DEFINE_GUID(GUID_COMPUTE_DEVICE_ARRIVAL, 0x1024e4c9, 0x47c9, 0x48d3, 0xb4, 0xa8, 0xf9, 0xdf, 0x78, 0x52, 0x3b, 0x53);

// D3DKMT_HANDLE is a 32-bit graphics handle, distinct from an NT process HANDLE.
typedef D3DKMT_HANDLE* PD3DKMT_HANDLE;

/**
 * Pre-Windows 8 segment statistics returned in D3DKMT_QUERYSTATISTICS.QueryResult.
 * The three leading byte counters are 32-bit values. The fixed layout is 144 bytes on x86
 * and x64 with 8-byte alignment. SystemMemoryEndAddress and PowerFlags belong to the Windows 8
 * layout, not this version. The corresponding legacy tail consists of eight reserved UINT64s.
 */
typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1
{
    ULONG CommitLimit;
    ULONG BytesCommitted;
    ULONG BytesResident;
    D3DKMT_QUERYSTATISTICS_MEMORY Memory;
    ULONG Aperture; // boolean
    D3DKMT_ALIGN64 ULONGLONG TotalBytesEvictedByPriority[D3DKMT_MaxAllocationPriorityClass];
    D3DKMT_ALIGN64 ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1, *PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1;

static_assert(sizeof(D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1) == 0x90, "D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1 size incorrect");
static_assert(UFIELD_OFFSET(D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1, Memory) == 0x10, "D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1.Memory offset incorrect");
static_assert(UFIELD_OFFSET(D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1, Reserved) == 0x50, "D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1.Reserved offset incorrect");

// rev
/**
 * The D3DKMT_GET_PROCESS_LIST structure is used for retrieving a list of process handles using a graphics adapter.
 * \remarks The caller is responsible for closing the returned process handles.
 * The structure is 24 bytes on x64 and 20 bytes on x86; the pointed-to elements are native HANDLEs,
 * not D3DKMT_HANDLE values. See D3DKMTGetProcessList for the WOW64 restriction.
 */
typedef struct _D3DKMT_GET_PROCESS_LIST
{
    LUID AdapterLuid;          // [in] The locally unique identifier (LUID) for the graphics adapter.
    ULONG DesiredAccess;       // [in] The access rights to request for the process handles. This must be `PROCESS_QUERY_INFORMATION` (0x400).
    ULONG ProcessHandleCount;  // [in, out] Capacity in HANDLE elements; required capacity on STATUS_BUFFER_TOO_SMALL, returned count on success.
    PHANDLE ProcessHandle;     // [in] Caller-supplied pointer to the output HANDLE array; NULL queries capacity.
} D3DKMT_GET_PROCESS_LIST, *PD3DKMT_GET_PROCESS_LIST;

// rev
/**
 * The D3DKMTGetProcessList function retrieves a list of processes that are using a specific graphics adapter.
 *
 * \param[in,out] GetProcessList A pointer to a \ref D3DKMT_GET_PROCESS_LIST structure that contains the processes using the graphics adapter.
 * \return NTSTATUS Successful or errant status.
 * \remarks Set ProcessHandle to NULL and ProcessHandleCount to zero to query
 * the required capacity. STATUS_BUFFER_TOO_SMALL updates ProcessHandleCount
 * without returning a partial handle array.
 * On success, ProcessHandleCount counts only successfully opened handles;
 * processes whose handles cannot be opened are skipped. The caller must close
 * every returned handle. The process list can change between calls, so a retry
 * can require a larger array. The capacity includes adapter-list entries that can be skipped,
 * so it can exceed the successful handle count even if the list does not change.
 * A NULL buffer returns STATUS_BUFFER_TOO_SMALL even when the required capacity is zero.
 * Invalid DesiredAccess values return STATUS_INVALID_PARAMETER; an adapter without a render
 * component returns STATUS_NOT_SUPPORTED. Access checks are performed when opening each process.
 * \warning The WOW64 thunk marshals only one HANDLE rather than the array.
 * Do not retrieve multi-handle arrays through that thunk. Use D3DKMTEnumProcesses from a WOW64
 * caller, or retrieve handles in a native 64-bit process.
 */
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetProcessList(
    _Inout_ PD3DKMT_GET_PROCESS_LIST GetProcessList
    );

// rev
/**
 * The D3DKMT_ENUM_PROCESS_LIST structure is used for retrieving a list of process identifiers using a graphics adapter.
 * The structure is 24 bytes on x64 and 16 bytes on x86. ProcessIdCount is pointer-sized,
 * but each output process identifier is always a 4-byte ULONG.
 */
typedef struct _D3DKMT_ENUM_PROCESS_LIST
{
    LUID AdapterLuid;          // [in] The locally unique identifier (LUID) for the graphics adapter.
    PULONG ProcessIdBuffer;    // [in] Caller-supplied pointer to the output ULONG array; NULL queries count.
    SIZE_T ProcessIdCount;     // [in, out] Capacity in ULONG elements; required count on STATUS_BUFFER_TOO_SMALL, returned count on success.
} D3DKMT_ENUM_PROCESS_LIST, *PD3DKMT_ENUM_PROCESS_LIST;

// rev
/**
 * The D3DKMTEnumProcesses function retrieves the process IDs associated with a graphics adapter,
 * without opening process handles.
 *
 * \param[in,out] EnumProcessList A pointer to a \ref D3DKMT_ENUM_PROCESS_LIST structure that contains the processes using the graphics adapter.
 * \return NTSTATUS Successful or errant status.
 * \remarks Set ProcessIdBuffer to NULL and ProcessIdCount to zero to query
 * the required count. A NULL or undersized buffer returns
 * STATUS_BUFFER_TOO_SMALL and updates ProcessIdCount; no partial PID array
 * is returned. A NULL buffer returns STATUS_BUFFER_TOO_SMALL even when the
 * required count is zero.
 * Input counts greater than 0x3FFFFFFF return STATUS_INVALID_PARAMETER.
 * Process IDs are ULONG values, not handles, and must not be closed.
 * The required count can change between calls; retry if the buffer becomes too small.
 * Unlike D3DKMTGetProcessList, this enumeration does not filter entries by whether a process
 * handle can be opened. An invalid adapter or one without a render component returns
 * STATUS_INVALID_PARAMETER.
 */
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTEnumProcesses(
    _Inout_ PD3DKMT_ENUM_PROCESS_LIST EnumProcessList
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
// rev
/**
 * The NtDirectGraphicsCall routine invokes a direct graphics kernel call.
 *
 * \param InputBufferLength The length, in bytes, of the input buffer.
 * \param InputBuffer An optional pointer to the input buffer.
 * \param OutputBufferLength The length, in bytes, of the output buffer.
 * \param OutputBuffer An optional pointer to the output buffer that receives call results.
 * \param ReturnLength A pointer to a variable for the returned data size, in bytes.
 * \return NTSTATUS Successful or errant status.
 * \remarks This service unconditionally returns STATUS_NOT_IMPLEMENTED without accessing 
 * any argument or updating OutputBuffer or ReturnLength.
 */
_Kernel_entry_
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

//
// Private D3DKMT exports absent from SDK 10.0.28000.0.
//
/**
 * Additional Gdi32 exports resolved through DXCore/Win32u on Windows resolve these names 
 * dynamically when supporting older systems; export presence does not imply that the 
 * operation is supported by the current adapter, session or process.
 *
 * PVOID Data parameters deliberately denote opaque, operation-specific private structures.
 * Their complete layouts are not declared here. Do not pass an arbitrary buffer or assume
 * that different Data parameters have interchangeable layouts or sizes. These declarations
 * describe the exported calling interfaces, not a stable private-buffer ABI.

// rev
/**
 * Pointer-free adapter record returned by D3DKMTDDisplayEnum (568 bytes, 4-byte alignment).
 * AdapterData contains seven unclassified 32-bit metadata fields. Flags is a private bitmap.
 */
typedef struct _D3DKMT_DDISPLAY_ENUM_ADAPTER
{
    LUID AdapterLuid;
    LUID PairedAdapterLuid;
    WCHAR DeviceName[260];
    ULONG AdapterData[7];
    ULONG Flags;
} D3DKMT_DDISPLAY_ENUM_ADAPTER, *PD3DKMT_DDISPLAY_ENUM_ADAPTER;

// rev
/**
 * Pointer-free target record returned by D3DKMTDDisplayEnum (1088 bytes, 4-byte alignment).
 * TargetData contains four unclassified 32-bit metadata fields. MonitorType and Flags use
 * private values; the two orientation fields use D3DKMDT_MONITOR_ORIENTATION values.
 */
typedef struct _D3DKMT_DDISPLAY_ENUM_TARGET
{
    LUID AdapterLuid;
    D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId;
    WCHAR MonitorId[260];
    WCHAR DeviceInterface[260];
    ULONG OutputTechnology;
    ULONG MonitorType;
    ULONG TargetData[4];
    D3DKMDT_MONITOR_ORIENTATION Orientation;
    D3DKMDT_MONITOR_ORIENTATION NativeOrientation;
    ULONG Flags;
} D3DKMT_DDISPLAY_ENUM_TARGET, *PD3DKMT_DDISPLAY_ENUM_TARGET;

// rev
/**
 * Set both capacities to zero and both array pointers to NULL for a sizing query.
 * AdapterCount and TargetCount receive the required counts, including on STATUS_BUFFER_TOO_SMALL.
 * Arrays are copied to user mode only on success; capacities are element counts, not byte sizes.
 * The envelope is 32 bytes on x64 and 24 bytes on x86. Array records have the same layout on both.
 */
typedef struct _D3DKMT_DDISPLAY_ENUM
{
    ULONG AdapterCount;
    ULONG AdapterCapacity;
    PD3DKMT_DDISPLAY_ENUM_ADAPTER Adapters;
    ULONG TargetCount;
    ULONG TargetCapacity;
    PD3DKMT_DDISPLAY_ENUM_TARGET Targets;
} D3DKMT_DDISPLAY_ENUM, *PD3DKMT_DDISPLAY_ENUM;

// rev
typedef struct _D3DKMT_DDISPLAY_ENUM_ADAPTER2
{
    ULONG Reserved;
    D3DKMT_DDISPLAY_ENUM_ADAPTER Adapter;
} D3DKMT_DDISPLAY_ENUM_ADAPTER2, *PD3DKMT_DDISPLAY_ENUM_ADAPTER2;

// rev
typedef struct _D3DKMT_DDISPLAY_ENUM_TARGET2
{
    ULONG Reserved;
    D3DKMT_DDISPLAY_ENUM_TARGET Target;
    ULONG TargetData2; // unclassified metadata at offset 1092
} D3DKMT_DDISPLAY_ENUM_TARGET2, *PD3DKMT_DDISPLAY_ENUM_TARGET2;

#define D3DKMT_DDISPLAY_ENUM_VERSION_1 1

// rev
/**
 * Versioned display enumeration: 48-byte envelope on x64 and 36 bytes on x86.
 * Initialize Version to D3DKMT_DDISPLAY_ENUM_VERSION_1, AdapterSize to 572 and TargetSize to 1096.
 * Counts, capacities and sizing queries follow D3DKMT_DDISPLAY_ENUM. Size fields are array strides.
 * The current implementation copies at most 572/1096 bytes per record when larger strides are used.
 * D3DKMTDDisplayEnum2 returns STATUS_NOT_IMPLEMENTED when its feature is disabled.
 */
typedef struct _D3DKMT_DDISPLAY_ENUM2
{
    ULONG Version;
    ULONG AdapterCount;
    ULONG AdapterCapacity;
    ULONG AdapterSize;
    PD3DKMT_DDISPLAY_ENUM_ADAPTER2 Adapters;
    ULONG TargetCount;
    ULONG TargetCapacity;
    ULONG TargetSize;
    PD3DKMT_DDISPLAY_ENUM_TARGET2 Targets;
} D3DKMT_DDISPLAY_ENUM2, *PD3DKMT_DDISPLAY_ENUM2;

// rev
typedef enum _D3DKMT_DISPLAYPORT_OPERATION_TYPE
{
    D3DKMTDisplayPortQueryCaps = 1,       // D3DKMT_DP_QUERY_CAPS; minimum total size 24
    D3DKMTDisplayPortGetAddress = 2,      // D3DKMT_DP_GET_ADDRESS; minimum total size 44
    D3DKMTDisplayPortAuxIo = 3,           // D3DKMT_DP_AUX_IO; minimum total size 56
    D3DKMTDisplayPortI2cIo = 4,           // D3DKMT_DP_I2C_IO; minimum total size 60
    D3DKMTDisplayPortSideband = 5         // D3DKMT_DP_SIDEBAND; minimum total size 60
} D3DKMT_DISPLAYPORT_OPERATION_TYPE;

// User-mode payload counterparts of the DisplayPort interface packets in WDK dispmprt.h.
// These declarations avoid including the kernel-only interface/callback dependencies.
typedef struct _D3DKMT_DP_QUERY_CAPS
{
    ULONG NumRootPorts;
    UCHAR DPVersionMajor;
    UCHAR DPVersionMinor;
} D3DKMT_DP_QUERY_CAPS, *PD3DKMT_DP_QUERY_CAPS;

typedef struct _D3DKMT_DP_GET_ADDRESS
{
    D3DDDI_VIDEO_PRESENT_TARGET_ID TargetId;
    ULONG DPNativeError;
    ULONG RootPortIndex;
    UCHAR NumLinks;
    UCHAR RelAddress[15];
} D3DKMT_DP_GET_ADDRESS, *PD3DKMT_DP_GET_ADDRESS;

#define D3DKMT_DP_AUX_WRITE 0x00000001
#define D3DKMT_DP_AUX_CAN_USE_CACHED_DATA 0x00000002

typedef struct _D3DKMT_DP_AUX_IO
{
    ULONG Flags; // D3DKMT_DP_AUX_*
    ULONG RootPortIndex;
    ULONG DPCDAddress;
    UCHAR NumBytesRequested;
    ULONG DPNativeError;
    UCHAR NumBytesDone;
    UCHAR Data[16];
} D3DKMT_DP_AUX_IO, *PD3DKMT_DP_AUX_IO;

#define D3DKMT_DP_I2C_READ 0x00000001
#define D3DKMT_DP_I2C_WRITE 0x00000002
#define D3DKMT_DP_I2C_EDDC_MODE 0x00000004
#define D3DKMT_DP_I2C_OFFSET_SIZE_SHIFT 3
#define D3DKMT_DP_I2C_OFFSET_SIZE_MASK 0x00000038
#define D3DKMT_DP_I2C_CAN_USE_CACHED_DATA 0x00000040

typedef struct _D3DKMT_DP_I2C_IO
{
    ULONG Flags; // D3DKMT_DP_I2C_*
    ULONG RootPortIndex;
    ULONG I2CAddress;
    ULONG Offset; // EDDC mode: word offset in bits 0..7, segment pointer in bits 8..14
    ULONG BufferSizeSupplied;
    ULONG BytesToWrite;
    ULONG BytesToRead;
    ULONG DPNativeError;
    ULONG BytesWritten;
    ULONG BytesRead;
    UCHAR Data[ANYSIZE_ARRAY];
} D3DKMT_DP_I2C_IO, *PD3DKMT_DP_I2C_IO;

#define D3DKMT_DP_SIDEBAND_CAN_USE_CACHED_DATA 0x00000001

typedef struct _D3DKMT_DP_SIDEBAND
{
    ULONG Flags; // D3DKMT_DP_SIDEBAND_*
    ULONG RootPortIndex;
    ULONG BufferSizeSupplied;
    ULONG RequestLength;
    ULONG MaxReplyLength;
    ULONG DPNativeError;
    ULONG ActualReplyLength;
    UCHAR Data[ANYSIZE_ARRAY];
} D3DKMT_DP_SIDEBAND, *PD3DKMT_DP_SIDEBAND;

// rev
/**
 * A 16-byte header followed immediately by the selected DisplayPort payload; no payload pointer.
 * Size includes the header and any variable data, and must be in the range 17..4112 bytes.
 * The operation-specific minimums are listed in D3DKMT_DISPLAYPORT_OPERATION_TYPE.
 *
 * The caller must be an administrator or LocalSystem. AUX writes are rejected and AUX requests
 * are limited to 16 bytes. I2C read/write lengths cannot exceed BufferSizeSupplied; writes require
 * address 0x6e, offset sizes are limited to 4 bytes, and EDDC mode requires a 1-byte offset.
 * For I2C and sideband requests, allocate enough trailing storage for BufferSizeSupplied bytes.
 * Sideband messages are additionally filtered by the kernel. The miniport result is returned as
 * NTSTATUS; the payload also contains DPNativeError. The payload is copied back after the DDI call.
 */
typedef struct _D3DKMT_DISPLAYPORT_OPERATION
{
    D3DKMT_DISPLAYPORT_OPERATION_TYPE Operation;
    ULONG Size;
    LUID AdapterLuid;
    union
    {
        D3DKMT_DP_QUERY_CAPS QueryCaps;
        D3DKMT_DP_GET_ADDRESS GetAddress;
        D3DKMT_DP_AUX_IO AuxIo;
        D3DKMT_DP_I2C_IO I2cIo;
        D3DKMT_DP_SIDEBAND Sideband;
        UCHAR Data[ANYSIZE_ARRAY];
    };
} D3DKMT_DISPLAYPORT_OPERATION, *PD3DKMT_DISPLAYPORT_OPERATION;

#define D3DKMT_DUPLICATEHANDLE_SAME_ACCESS 0x00000001

// rev
/**
 * Duplicates a graphics shared-allocation NT handle into a process in a guest VM.
 * This is not a general-purpose DuplicateHandle replacement. SourceHandle needs READ_CONTROL.
 * VirtualMachineId must be nonzero. TargetProcessId is a guest process ID, not a process handle.
 * Only SAME_ACCESS is accepted in Flags; when set, DesiredAccess must be zero.
 * The returned TargetHandle belongs to the guest process and must not be closed in the caller.
 * Layout: 48 bytes on x64, 36 bytes on x86. Handles are native-width, not D3DKMT_HANDLE values.
 */
typedef struct _D3DKMT_DUPLICATEHANDLE
{
    HANDLE SourceHandle;
    GUID VirtualMachineId;
    HANDLE TargetProcessId;
    HANDLE TargetHandle;
    ACCESS_MASK DesiredAccess;
    ULONG Flags; // D3DKMT_DUPLICATEHANDLE_*
} D3DKMT_DUPLICATEHANDLE, *PD3DKMT_DUPLICATEHANDLE;

#define D3DKMT_GETPROCESSSCHEDULINGPRIORITYBAND_SIZE 24

// rev
/**
 * Queries the private process scheduling band (0 or 1 in the examined implementation).
 * These values are not the DXGK_SCHEDULING_PRIORITY_BAND miniport enumeration.
 * Flags must be zero; ProcessHandle requires PROCESS_SET_LIMITED_INFORMATION (0x2000).
 * The caller must pass the graphics resource-manager token-membership check.
 *
 * Set Size to D3DKMT_GETPROCESSSCHEDULINGPRIORITYBAND_SIZE on both x64 and WOW64.
 * The x86 envelope is 16 bytes, but the WOW64 thunk preserves Size and the native handler requires
 * at least 24. The native envelope is 24 bytes; only PriorityBand is an output field.
 */
typedef struct _D3DKMT_GETPROCESSSCHEDULINGPRIORITYBAND
{
    ULONG Size;
    ULONG Flags;
    HANDLE ProcessHandle;
    ULONG PriorityBand;
} D3DKMT_GETPROCESSSCHEDULINGPRIORITYBAND, *PD3DKMT_GETPROCESSSCHEDULINGPRIORITYBAND;

static_assert(sizeof(D3DKMT_DDISPLAY_ENUM_ADAPTER) == 0x238, "D3DKMT_DDISPLAY_ENUM_ADAPTER size incorrect");
static_assert(sizeof(D3DKMT_DDISPLAY_ENUM_TARGET) == 0x440, "D3DKMT_DDISPLAY_ENUM_TARGET size incorrect");
static_assert(sizeof(D3DKMT_DDISPLAY_ENUM_ADAPTER2) == 0x23c, "D3DKMT_DDISPLAY_ENUM_ADAPTER2 size incorrect");
static_assert(sizeof(D3DKMT_DDISPLAY_ENUM_TARGET2) == 0x448, "D3DKMT_DDISPLAY_ENUM_TARGET2 size incorrect");
static_assert(sizeof(D3DKMT_DISPLAYPORT_OPERATION) == 0x3c, "D3DKMT_DISPLAYPORT_OPERATION size incorrect");
static_assert(UFIELD_OFFSET(D3DKMT_DISPLAYPORT_OPERATION, Data) == 0x10, "D3DKMT_DISPLAYPORT_OPERATION.Data offset incorrect");

#ifdef _WIN64
static_assert(sizeof(D3DKMT_DDISPLAY_ENUM) == 0x20, "D3DKMT_DDISPLAY_ENUM size incorrect");
static_assert(sizeof(D3DKMT_DDISPLAY_ENUM2) == 0x30, "D3DKMT_DDISPLAY_ENUM2 size incorrect");
static_assert(sizeof(D3DKMT_DUPLICATEHANDLE) == 0x30, "D3DKMT_DUPLICATEHANDLE size incorrect");
static_assert(sizeof(D3DKMT_GETPROCESSSCHEDULINGPRIORITYBAND) == 0x18, "D3DKMT_GETPROCESSSCHEDULINGPRIORITYBAND size incorrect");
#else
static_assert(sizeof(D3DKMT_DDISPLAY_ENUM) == 0x18, "D3DKMT_DDISPLAY_ENUM size incorrect");
static_assert(sizeof(D3DKMT_DDISPLAY_ENUM2) == 0x24, "D3DKMT_DDISPLAY_ENUM2 size incorrect");
static_assert(sizeof(D3DKMT_DUPLICATEHANDLE) == 0x24, "D3DKMT_DUPLICATEHANDLE size incorrect");
static_assert(sizeof(D3DKMT_GETPROCESSSCHEDULINGPRIORITYBAND) == 0x10, "D3DKMT_GETPROCESSSCHEDULINGPRIORITYBAND size incorrect");
#endif

struct _D3DKMT_GETAVAILABLETRACKEDWORKLOADINDEX;

// rev
// NtGdiDdDDIAbandonSwapChain.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTAbandonSwapChain(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIAcquireSwapChain.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTAcquireSwapChain(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIAddSurfaceToSwapChain.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTAddSurfaceToSwapChain(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDICacheHybridQueryValue.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTCacheHybridQueryValue(
    _Inout_ PVOID Data
    );

// rev
// NtDxgkCheckSinglePlaneForMultiPlaneOverlaySupport.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTCheckSinglePlaneForMultiPlaneOverlaySupport(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDICreateBundleObject.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTCreateBundleObject(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDICreateSwapChain.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTCreateSwapChain(
    _Inout_ PVOID Data
    );

// rev
// Not present in SDK 10.0.28000.0 Gdi32.lib; resolve from Gdi32.dll with GetProcAddress.
// NtDxgkCreateTrackedWorkload.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTCreateTrackedWorkload(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIDDisplayEnum.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDDisplayEnum(
    _Inout_ PD3DKMT_DDISPLAY_ENUM Data
    );

// rev
// NtDxgkDDisplayEnum2.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDDisplayEnum2(
    _Inout_ PD3DKMT_DDISPLAY_ENUM2 Data
    );

// rev
// Not present in SDK 10.0.28000.0 Gdi32.lib; resolve from Gdi32.dll with GetProcAddress.
// NtDxgkDestroyTrackedWorkload.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDestroyTrackedWorkload(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIDispMgrCreate.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDispMgrCreate(
    _Inout_ PVOID Data
    );

// rev
// Not present in SDK 10.0.28000.0 Gdi32.lib; resolve from Gdi32.dll with GetProcAddress.
// NtDxgkDispMgrOperation.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDispMgrOperation(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIDispMgrSourceOperation.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDispMgrSourceOperation(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIDispMgrTargetOperation.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDispMgrTargetOperation(
    _Inout_ PVOID Data
    );

// rev
// NtDxgkDisplayMuxSwitchExecute.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDisplayMuxSwitchExecute(
    _Inout_ PVOID Data
    );

// rev
// NtDxgkDisplayMuxSwitchFinish.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDisplayMuxSwitchFinish(
    _Inout_ PVOID Data
    );

// rev
// NtDxgkDisplayMuxSwitchPrepare.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDisplayMuxSwitchPrepare(
    _Inout_ PVOID Data
    );

// rev
// Not present in SDK 10.0.28000.0 Gdi32.lib; resolve from Gdi32.dll with GetProcAddress.
// NtDxgkDisplayPortOperation.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDisplayPortOperation(
    _Inout_ PD3DKMT_DISPLAYPORT_OPERATION Data
    );

// rev
// NtDxgkDuplicateHandle.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTDuplicateHandle(
    _Inout_ PD3DKMT_DUPLICATEHANDLE Data
    );

// rev
// NtGdiDdDDIExtractBundleObject.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTExtractBundleObject(
    _Inout_ PVOID Data
    );

// rev
// Not present in SDK 10.0.28000.0 Gdi32.lib; resolve from Gdi32.dll with GetProcAddress.
// NtDxgkGetAvailableTrackedWorkloadIndex.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetAvailableTrackedWorkloadIndex(
    _Inout_ struct _D3DKMT_GETAVAILABLETRACKEDWORKLOADINDEX *Data
    );

// rev
// NtGdiDdDDIGetCachedHybridQueryValue.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetCachedHybridQueryValue(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIGetMemoryBudgetTarget.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetMemoryBudgetTarget(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIGetProcessSchedulingPriorityBand.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetProcessSchedulingPriorityBand(
    _Inout_ PD3DKMT_GETPROCESSSCHEDULINGPRIORITYBAND Data
    );

// rev
// NtDxgkGetProperties.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetProperties(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIGetSetSwapChainMetadata.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetSetSwapChainMetadata(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIGetSwapChainSurfacePhysicalAddress.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetSwapChainSurfacePhysicalAddress(
    _Inout_ PVOID Data
    );

// rev
// Not present in SDK 10.0.28000.0 Gdi32.lib; resolve from Gdi32.dll with GetProcAddress.
// NtDxgkGetTrackedWorkloadStatistics.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetTrackedWorkloadStatistics(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIGetYieldPercentage.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetYieldPercentage(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDINetDispGetNextChunkInfo.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTNetDispGetNextChunkInfo(
    _In_reads_bytes_(8) PCVOID Context,
    _In_ ULONG Flags,
    _In_range_(0, 4) ULONG EventCount,
    _In_reads_(EventCount) const HANDLE *Events,
    _Inout_ PULONG BufferSize,
    _Out_writes_bytes_(*BufferSize) PVOID Buffer,
    _Out_ PULONG ChunkInfo
    );

// rev
// Returns a private 32-bit Miracast result code, not a documented NTSTATUS.
// NtGdiDdDDINetDispQueryMiracastDisplayDeviceStatus.
NTSYSAPI
LONG
NTAPI
D3DKMTNetDispQueryMiracastDisplayDeviceStatus(
    _In_ PCWSTR DeviceName,
    _Out_ PULONG DeviceStatus
    );

// rev
// NtGdiDdDDINetDispQueryMiracastDisplayDeviceSupport.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTNetDispQueryMiracastDisplayDeviceSupport(
    _Out_writes_bytes_(12) PVOID Data
    );

// rev
// Equivalent to D3DKMTNetDispStartMiracastDisplayDevice2 with Flags = 1.
// Returns a private 32-bit Miracast result code, not a documented NTSTATUS.
/**
 * Starts a Miracast display through the version-2 client wrapper with Flags=1. DeviceName must be a readable NUL-
 * terminated string. The wrapper forwards EventHandle and returns the version-2 result; it is not a read-only
 * display query. See D3DKMTNetDispStartMiracastDisplayDevice2 for the local length rejection.
 */
NTSYSAPI
LONG
NTAPI
D3DKMTNetDispStartMiracastDisplayDevice(
    _In_ PCWSTR DeviceName,
    _In_opt_ HANDLE EventHandle
    );

// rev
// DeviceName is copied into a 260-WCHAR field; Flags is passed through unchanged.
// Returns a private 32-bit Miracast result code, not a documented NTSTATUS.
/**
 * Builds a Miracast start request and submits it to Win32u. The request size is 552 bytes on x64 and 548 bytes on
 * x86. A readable NUL-terminated DeviceName of 261 or more WCHARs, excluding NUL, is rejected locally with
 * 0x80000008 before the native request. Shorter strings reach a 260-WCHAR destination copy; this is not a promise
 * that all shorter strings are valid device names. Flags and EventHandle are forwarded. Successful dispatch can
 * change display state.
 */
NTSYSAPI
LONG
NTAPI
D3DKMTNetDispStartMiracastDisplayDevice2(
    _In_ PCWSTR DeviceName,
    _In_ ULONG Flags,
    _In_opt_ HANDLE EventHandle
    );

// rev
// Returns a private 32-bit Miracast result code, not a documented NTSTATUS.
// NtGdiDdDDINetDispStartMiracastDisplayDevice.
NTSYSAPI
LONG
NTAPI
D3DKMTNetDispStartMiracastDisplayDeviceEx(
    _In_ PVOID Data
    );

// rev
// Returns a private 32-bit Miracast result code, not a documented NTSTATUS.
// NtGdiDdDDINetDispStopMiracastDisplayDevice.
NTSYSAPI
LONG
NTAPI
D3DKMTNetDispStopMiracastDisplayDevice(
    _In_ PCWSTR DeviceName,
    _In_opt_ HANDLE EventHandle
    );

// rev
// NtGdiDdDDIOpenBundleObjectNtHandleFromName.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTOpenBundleObjectNtHandleFromName(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIOpenSwapChain.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTOpenSwapChain(
    _Inout_ PVOID Data
    );

// rev
// Reads the legacy direct-flip parameter block: 16 bytes on x64, 12 bytes on x86.
/**
 * Pins the ResourceCount graphics resources in Resources->pResourceList for Resources->hDevice. Uses the SDK
 * D3DKMT_PINDIRECTFLIPRESOURCES type. The client reads the device at offset 0, count at offset 4, and pointer at
 * offset 8, then builds a private 32-byte request with pin flag bit 0 set and submits NtDxgkPinResources. It does
 * not copy the request back to Resources. Kernel validation and resource lifetime remain separate contracts.
 */
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTPinDirectFlipResources(
    _In_ const D3DKMT_PINDIRECTFLIPRESOURCES *Resources
    );

// rev
// NtDxgkPinResources.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTPinResources(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIReleaseSwapChain.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTReleaseSwapChain(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIRemoveSurfaceFromSwapChain.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTRemoveSurfaceFromSwapChain(
    _Inout_ PVOID Data
    );

// rev
// Not present in SDK 10.0.28000.0 Gdi32.lib; resolve from Gdi32.dll with GetProcAddress.
// NtDxgkResetTrackedWorkloadStatistics.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTResetTrackedWorkloadStatistics(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDISetDodIndirectSwapchain.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTSetDodIndirectSwapchain(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDISetMemoryBudgetTarget.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTSetMemoryBudgetTarget(
    _Inout_ PVOID Data
    );

// rev
// Enable points to one BOOLEAN, not a BOOL or a by-value flag.
// NtGdiDdDDISetProcessDeviceRemovalSupport.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTSetProcessDeviceRemovalSupport(
    _In_ PBOOLEAN Enable
    );

// rev
// NtGdiDdDDISetProcessSchedulingPriorityBand.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTSetProcessSchedulingPriorityBand(
    _Inout_ PVOID Data
    );

// rev
// NtDxgkSetProperties.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTSetProperties(
    _Inout_ PVOID Data
    );

// rev
// Enabled is passed by value, unlike the private parameter-block APIs.
// NtGdiDdDDISetStereoEnabled.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTSetStereoEnabled(
    _In_ BOOL Enabled
    );

// rev
// NtGdiDdDDISetYieldPercentage.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTSetYieldPercentage(
    _Inout_ PVOID Data
    );

// rev
// NtGdiDdDDIUnOrderedPresentSwapChain.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTUnOrderedPresentSwapChain(
    _Inout_ PVOID Data
    );

// rev
// Reads the legacy direct-flip parameter block: 16 bytes on x64, 12 bytes on x86.
/**
 * Unpins resources described by the SDK D3DKMT_UNPINDIRECTFLIPRESOURCES record. DXCore reads hDevice at offset
 * 0, ResourceCount at offset 4 and pResourceList at offset 8, builds a private request with additional flag
 * storage cleared, and calls NtDxgkUnpinResources without writing back to Resources. This changes resource
 * pinning state; device and resource lifetimes must be valid.
 */
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTUnpinDirectFlipResources(
    _In_ const D3DKMT_UNPINDIRECTFLIPRESOURCES *Resources
    );

// rev
// NtDxgkUnpinResources.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTUnpinResources(
    _Inout_ PVOID Data
    );

// rev
// Not present in SDK 10.0.28000.0 Gdi32.lib; resolve from Gdi32.dll with GetProcAddress.
// NtDxgkUpdateTrackedWorkload.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTUpdateTrackedWorkload(
    _Inout_ PVOID Data
    );

// rev
// Operates on the current graphics process; takes no arguments.
// NtDxgkVailConnect.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTVailConnect(
    VOID
    );

// rev
// Operates on the current graphics process; takes no arguments.
// NtDxgkVailDisconnect.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTVailDisconnect(
    VOID
    );

// rev
// NtDxgkVailPromoteCompositionSurface.
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTVailPromoteCompositionSurface(
    _In_ HANDLE CompositionSurface,
    _Out_ PHANDLE PromotedCompositionSurface
    );

EXTERN_C_END

#endif // _NTD3DKMT_H

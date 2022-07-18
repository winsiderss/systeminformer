#ifndef KPH_H
#define KPH_H

#include <ntifs.h>
#include <ntintsafe.h>
#define PHNT_MODE PHNT_MODE_KERNEL
#include <phnt.h>
#include <ntfill.h>
#include <bcrypt.h>
#include <kphapi.h>

#ifdef DBG
#define PAGED_PASSIVE() PAGED_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL)
#else
#define PAGED_PASSIVE()
#endif

// Memory

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))
#define PTR_SUB_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) - (ULONG_PTR)(Offset)))

// Zero extension and sign extension macros

#define C_2sTo4(x) ((unsigned int)(signed short)(x))

// Debugging

#ifdef DBG
#define dprintf(Format, ...) DbgPrint("KProcessHacker: " Format, __VA_ARGS__)
#else
#define dprintf
#endif

typedef struct _KPH_EXTENTS
{
    PVOID BaseAddress;
    PVOID EndAddress;
} KPH_EXTENTS, *PKPH_EXTENTS;

typedef struct _KPH_CLIENT
{
    struct
    {
        ULONG VerificationPerformed : 1;
        ULONG VerificationSucceeded : 1;
        ULONG KeysGenerated : 1;
        ULONG SpareBits : 29;
    };
    FAST_MUTEX StateMutex;
    NTSTATUS VerificationStatus;
    PVOID VerifiedProcess; // EPROCESS (for equality checking only - do not access contents)
    HANDLE VerifiedProcessId;
    PVOID VerifiedRangeBase;
    SIZE_T VerifiedRangeSize;
    // Level 1 and 2 secret keys
    FAST_MUTEX KeyBackoffMutex;
    KPH_KEY L1Key;
    KPH_KEY L2Key;
    KPH_EXTENTS VerifiedProcessExtents;
} KPH_CLIENT, *PKPH_CLIENT;

// main

extern ULONG KphFeatures;
extern KPH_EXTENTS NtdllExtents;

NTSTATUS KpiGetFeatures(
    _Out_ PULONG Features,
    _In_ KPROCESSOR_MODE AccessMode
    );

// devctrl

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH KphDispatchDeviceControl;

NTSTATUS KphDispatchDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    );

// dynimp

VOID KphDynamicImport(
    VOID
    );

PVOID KphGetSystemRoutineAddress(
    _In_ PWSTR SystemRoutineName
    );

// object

PHANDLE_TABLE KphReferenceProcessHandleTable(
    _In_ PEPROCESS Process
    );

VOID KphDereferenceProcessHandleTable(
    _In_ PEPROCESS Process
    );

VOID KphUnlockHandleTableEntry(
    _In_ PHANDLE_TABLE HandleTable,
    _In_ PHANDLE_TABLE_ENTRY HandleTableEntry
    );

NTSTATUS KpiEnumerateProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_opt_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphQueryNameObject(
    _In_ PVOID Object,
    _Out_writes_bytes_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    );

NTSTATUS KphQueryNameFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    );

NTSTATUS KpiQueryInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiSetInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _In_reads_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenNamedObject(
    _Out_ PHANDLE ObjectHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ POBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE AccessMode
    );

// process

NTSTATUS KpiOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiOpenProcessJob(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE JobHandle,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KpiGetProcessProtection(
    _In_ PEPROCESS Process,
    _Out_ PPS_PROTECTION Protection
    );

// qrydrv

NTSTATUS KpiOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiQueryInformationDriver(
    _In_ HANDLE DriverHandle,
    _In_ DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_writes_bytes_(DriverInformationLength) PVOID DriverInformation,
    _In_ ULONG DriverInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// thread

NTSTATUS KpiOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    );

#define KPH_STACK_TRACE_CAPTURE_USER_STACK 0x00000001 

_Success_(return != 0)
ULONG KphCaptureStackBackTrace(
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _In_opt_ ULONG Flags,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG BackTraceHash
    );

NTSTATUS KphCaptureStackBackTraceThread(
    _In_ PETHREAD Thread,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ ULONG Flags
    );

NTSTATUS KpiCaptureStackBackTraceThread(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Out_opt_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ ULONG Flags
    );

NTSTATUS KpiQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

// util

VOID KphFreeCapturedUnicodeString(
    _In_ PUNICODE_STRING CapturedUnicodeString
    );

NTSTATUS KphCaptureUnicodeString(
    _In_ PUNICODE_STRING UnicodeString,
    _Out_ PUNICODE_STRING CapturedUnicodeString
    );

NTSTATUS KphEnumerateSystemModules(
    _Out_ PRTL_PROCESS_MODULES *Modules
    );

NTSTATUS KphValidateAddressForSystemModules(
    _In_ PVOID Address,
    _In_ SIZE_T Length
    );

NTSTATUS KphGetProcessMappedFileName(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_ PUNICODE_STRING *FileName
    );

_IRQL_requires_min_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS
KphImageNtHeader(
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS* OutHeaders
    );

// verify

NTSTATUS KphHashFile(
    _In_ PUNICODE_STRING FileName,
    _Out_ PVOID *Hash,
    _Out_ PULONG HashSize
    );

NTSTATUS KphVerifyFile(
    _In_ PUNICODE_STRING FileName,
    _In_reads_bytes_(SignatureSize) PUCHAR Signature,
    _In_ ULONG SignatureSize
    );

VOID KphVerifyClient(
    _Inout_ PKPH_CLIENT Client,
    _In_ PVOID CodeAddress,
    _In_reads_bytes_(SignatureSize) PUCHAR Signature,
    _In_ ULONG SignatureSize
    );

NTSTATUS KpiVerifyClient(
    _In_ PVOID CodeAddress,
    _In_reads_bytes_(SignatureSize) PUCHAR Signature,
    _In_ ULONG SignatureSize,
    _In_ PKPH_CLIENT Client
    );

VOID KphGenerateKeysClient(
    _Inout_ PKPH_CLIENT Client
    );

NTSTATUS KphRetrieveKeyViaApc(
    _Inout_ PKPH_CLIENT Client,
    _In_ KPH_KEY_LEVEL KeyLevel,
    _Inout_ PIRP Irp
    );

NTSTATUS KphValidateKey(
    _In_ KPH_KEY_LEVEL RequiredKeyLevel,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphDominationCheck(
    _In_ PKPH_CLIENT Client,
    _In_ PEPROCESS Process,
    _In_ PEPROCESS ProcessTarget,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyCaller(
    _In_ PKPH_CLIENT Client,
    _In_ PETHREAD Thread,
    _In_ KPROCESSOR_MODE AccessMode
    );

// vm

NTSTATUS KphCopyVirtualMemory(
    _In_ PEPROCESS FromProcess,
    _In_ PVOID FromAddress,
    _In_ PEPROCESS ToProcess,
    _In_ PVOID ToAddress,
    _In_ SIZE_T BufferLength,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PSIZE_T ReturnLength
    );

NTSTATUS KpiReadVirtualMemoryUnsafe(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    );

#endif

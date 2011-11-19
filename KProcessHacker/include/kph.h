#ifndef KPH_H
#define KPH_H

#include <ntifs.h>
#define PHNT_MODE PHNT_MODE_KERNEL
#include <phnt.h>
#include <ntfill.h>
#include <kphapi.h>

// Configuration

// Disable features that conflict with driver signing requirements.
// KPH_CONFIG_CLEAN

// Debugging

#ifdef DBG
#define dprintf(Format, ...) DbgPrint("KProcessHacker: " Format, __VA_ARGS__)
#else
#define dprintf
#endif

typedef struct _KPH_PARAMETERS
{
    KPH_SECURITY_LEVEL SecurityLevel;
    LOGICAL DisableDynamicProcedureScan;
} KPH_PARAMETERS, *PKPH_PARAMETERS;

// main

extern ULONG KphFeatures;
extern KPH_PARAMETERS KphParameters;

NTSTATUS KpiGetFeatures(
    __out PULONG Features,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphEnumerateSystemModules(
    __out PRTL_PROCESS_MODULES *Modules
    );

NTSTATUS KphValidateAddressForSystemModules(
    __in PVOID Address,
    __in SIZE_T Length
    );

// devctrl

__drv_dispatchType(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH KphDispatchDeviceControl;

NTSTATUS KphDispatchDeviceControl(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    );

// dynimp

extern _ExfUnblockPushLock ExfUnblockPushLock_I;
extern _ObGetObjectType ObGetObjectType_I;
extern _PsAcquireProcessExitSynchronization PsAcquireProcessExitSynchronization_I;
extern _PsIsProtectedProcess PsIsProtectedProcess_I;
extern _PsReleaseProcessExitSynchronization PsReleaseProcessExitSynchronization_I;
extern _PsResumeProcess PsResumeProcess_I;
extern _PsSuspendProcess PsSuspendProcess_I;

VOID KphDynamicImport(
    VOID
    );

PVOID KphGetSystemRoutineAddress(
    __in PWSTR SystemRoutineName
    );

// object

POBJECT_TYPE KphGetObjectType(
    __in PVOID Object
    );

PHANDLE_TABLE KphReferenceProcessHandleTable(
    __in PEPROCESS Process
    );

VOID KphDereferenceProcessHandleTable(
    __in PEPROCESS Process
    );

VOID KphUnlockHandleTableEntry(
    __in PHANDLE_TABLE HandleTable,
    __in PHANDLE_TABLE_ENTRY HandleTableEntry
    );

NTSTATUS KpiEnumerateProcessHandles(
    __in HANDLE ProcessHandle,
    __out_bcount(BufferLength) PVOID Buffer,
    __in_opt ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphQueryNameObject(
    __in PVOID Object,
    __out_bcount(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    __in ULONG BufferLength,
    __out PULONG ReturnLength
    );

NTSTATUS KphQueryNameFileObject(
    __in PFILE_OBJECT FileObject,
    __out_bcount(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    __in ULONG BufferLength,
    __out PULONG ReturnLength
    );

NTSTATUS KpiQueryInformationObject(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __out_bcount(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiSetInformationObject(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __in_bcount(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphDuplicateObject(
    __in PEPROCESS SourceProcess,
    __in_opt PEPROCESS TargetProcess,
    __in HANDLE SourceHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiDuplicateObject(
    __in HANDLE SourceProcessHandle,
    __in HANDLE SourceHandle,
    __in_opt HANDLE TargetProcessHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphOpenNamedObject(
    __out PHANDLE ObjectHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in POBJECT_TYPE ObjectType,
    __in KPROCESSOR_MODE AccessMode
    );

// process

NTSTATUS KpiOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiOpenProcessToken(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE TokenHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiOpenProcessJob(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE JobHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiSuspendProcess(
    __in HANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiResumeProcess(
    __in HANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphTerminateProcessInternal(
    __in PEPROCESS Process,
    __in NTSTATUS ExitStatus
    );

NTSTATUS KpiTerminateProcess(
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiQueryInformationProcess(
    __in HANDLE ProcessHandle,
    __in KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiSetInformationProcess(
    __in HANDLE ProcessHandle,
    __in KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __in_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __in KPROCESSOR_MODE AccessMode
    );

BOOLEAN KphAcquireProcessRundownProtection(
    __in PEPROCESS Process
    );

VOID KphReleaseProcessRundownProtection(
    __in PEPROCESS Process
    );

// qrydrv

NTSTATUS KpiOpenDriver(
    __out PHANDLE DriverHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiQueryInformationDriver(
    __in HANDLE DriverHandle,
    __in DRIVER_INFORMATION_CLASS DriverInformationClass,
    __out_bcount(DriverInformationLength) PVOID DriverInformation,
    __in ULONG DriverInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

// thread

NTSTATUS KpiOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiOpenThreadProcess(
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KphTerminateThreadByPointerInternal(
    __in PETHREAD Thread,
    __in NTSTATUS ExitStatus
    );

NTSTATUS KpiTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiTerminateThreadUnsafe(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiGetContextThread(
    __in HANDLE ThreadHandle,
    __inout PCONTEXT ThreadContext,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiSetContextThread(
    __in HANDLE ThreadHandle,
    __in PCONTEXT ThreadContext,
    __in KPROCESSOR_MODE AccessMode
    );

ULONG KphCaptureStackBackTrace(
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __in_opt ULONG Flags,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG BackTraceHash
    );

NTSTATUS KphCaptureStackBackTraceThread(
    __in PETHREAD Thread,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiCaptureStackBackTraceThread(
    __in HANDLE ThreadHandle,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiQueryInformationThread(
    __in HANDLE ThreadHandle,
    __in KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiSetInformationThread(
    __in HANDLE ThreadHandle,
    __in KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    __in_bcount(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __in KPROCESSOR_MODE AccessMode
    );

// vm

NTSTATUS KphCopyVirtualMemory(
    __in PEPROCESS FromProcess,
    __in PVOID FromAddress,
    __in PEPROCESS ToProcess,
    __in PVOID ToAddress,
    __in SIZE_T BufferLength,
    __in KPROCESSOR_MODE AccessMode,
    __out PSIZE_T ReturnLength
    );

NTSTATUS KpiReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiWriteVirtualMemory(
    __in HANDLE ProcessHandle,
    __in_opt PVOID BaseAddress,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten,
    __in KPROCESSOR_MODE AccessMode
    );

NTSTATUS KpiReadVirtualMemoryUnsafe(
    __in_opt HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead,
    __in KPROCESSOR_MODE AccessMode
    );

// Inline support functions

FORCEINLINE VOID KphFreeCapturedUnicodeString(
    __in PUNICODE_STRING CapturedUnicodeString
    )
{
    if (CapturedUnicodeString->Buffer)
        ExFreePoolWithTag(CapturedUnicodeString->Buffer, 'UhpK');
}

FORCEINLINE NTSTATUS KphCaptureUnicodeString(
    __in PUNICODE_STRING UnicodeString,
    __out PUNICODE_STRING CapturedUnicodeString
    )
{
    UNICODE_STRING unicodeString;
    PWSTR userBuffer;

    __try
    {
        ProbeForRead(UnicodeString, sizeof(UNICODE_STRING), sizeof(ULONG));
        unicodeString.Length = UnicodeString->Length;
        unicodeString.MaximumLength = unicodeString.Length;
        unicodeString.Buffer = NULL;

        userBuffer = UnicodeString->Buffer;
        ProbeForRead(userBuffer, unicodeString.Length, sizeof(WCHAR));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    if (unicodeString.Length & 1)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (unicodeString.Length != 0)
    {
        unicodeString.Buffer = ExAllocatePoolWithTag(
            PagedPool,
            unicodeString.Length,
            'UhpK'
            );

        if (!unicodeString.Buffer)
            return STATUS_INSUFFICIENT_RESOURCES;

        __try
        {
            memcpy(
                unicodeString.Buffer,
                userBuffer,
                unicodeString.Length
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            KphFreeCapturedUnicodeString(&unicodeString);
            return GetExceptionCode();
        }
    }

    *CapturedUnicodeString = unicodeString;

    return STATUS_SUCCESS;
}

#endif

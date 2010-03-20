#ifndef KPH_H
#define KPH_H

#include <phbase.h>

#define KPH_DEVICE_TYPE (0x9999)
#define KPH_DEVICE_NAME (L"\\Device\\KProcessHacker")

#define KPHF_PSTERMINATEPROCESS 0x1
#define KPHF_PSPTERMINATETHREADBPYPOINTER 0x2

#define KPH_CTL_CODE(x) CTL_CODE(KPH_DEVICE_TYPE, 0x800 + x, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define KPH_CLOSEHANDLE KPH_CTL_CODE(0)
#define KPH_SSQUERYCLIENTENTRY KPH_CTL_CODE(1)
#define KPH_RESERVED1 KPH_CTL_CODE(2)
#define KPH_OPENPROCESS KPH_CTL_CODE(3)
#define KPH_OPENTHREAD KPH_CTL_CODE(4)
#define KPH_OPENPROCESSTOKEN KPH_CTL_CODE(5)
#define KPH_GETPROCESSPROTECTED KPH_CTL_CODE(6)
#define KPH_SETPROCESSPROTECTED KPH_CTL_CODE(7)
#define KPH_TERMINATEPROCESS KPH_CTL_CODE(8)
#define KPH_SUSPENDPROCESS KPH_CTL_CODE(9)
#define KPH_RESUMEPROCESS KPH_CTL_CODE(10)
#define KPH_READVIRTUALMEMORY KPH_CTL_CODE(11)
#define KPH_WRITEVIRTUALMEMORY KPH_CTL_CODE(12)
#define KPH_SETPROCESSTOKEN KPH_CTL_CODE(13)
#define KPH_GETTHREADSTARTADDRESS KPH_CTL_CODE(14)
#define KPH_SETHANDLEATTRIBUTES KPH_CTL_CODE(15)
#define KPH_GETHANDLEOBJECTNAME KPH_CTL_CODE(16)
#define KPH_OPENPROCESSJOB KPH_CTL_CODE(17)
#define KPH_GETCONTEXTTHREAD KPH_CTL_CODE(18)
#define KPH_SETCONTEXTTHREAD KPH_CTL_CODE(19)
#define KPH_GETTHREADWIN32THREAD KPH_CTL_CODE(20)
#define KPH_DUPLICATEOBJECT KPH_CTL_CODE(21)
#define KPH_ZWQUERYOBJECT KPH_CTL_CODE(22)
#define KPH_GETPROCESSID KPH_CTL_CODE(23)
#define KPH_GETTHREADID KPH_CTL_CODE(24)
#define KPH_TERMINATETHREAD KPH_CTL_CODE(25)
#define KPH_GETFEATURES KPH_CTL_CODE(26)
#define KPH_SETHANDLEGRANTEDACCESS KPH_CTL_CODE(27)
#define KPH_ASSIGNIMPERSONATIONTOKEN KPH_CTL_CODE(28)
#define KPH_PROTECTADD KPH_CTL_CODE(29)
#define KPH_PROTECTREMOVE KPH_CTL_CODE(30)
#define KPH_PROTECTQUERY KPH_CTL_CODE(31)
#define KPH_UNSAFEREADVIRTUALMEMORY KPH_CTL_CODE(32)
#define KPH_SETEXECUTEOPTIONS KPH_CTL_CODE(33)
#define KPH_QUERYPROCESSHANDLES KPH_CTL_CODE(34)
#define KPH_OPENTHREADPROCESS KPH_CTL_CODE(35)
#define KPH_CAPTURESTACKBACKTRACETHREAD KPH_CTL_CODE(36)
#define KPH_DANGEROUSTERMINATETHREAD KPH_CTL_CODE(37)
#define KPH_OPENTYPE KPH_CTL_CODE(38)
#define KPH_OPENDRIVER KPH_CTL_CODE(39)
#define KPH_QUERYINFORMATIONDRIVER KPH_CTL_CODE(40)
#define KPH_OPENDIRECTORYOBJECT KPH_CTL_CODE(41)
#define KPH_SSREF KPH_CTL_CODE(42)
#define KPH_SSUNREF KPH_CTL_CODE(43)
#define KPH_SSCREATECLIENTENTRY KPH_CTL_CODE(44)
#define KPH_SSCREATERULESETENTRY KPH_CTL_CODE(45)
#define KPH_SSREMOVERULE KPH_CTL_CODE(46)
#define KPH_SSADDPROCESSIDRULE KPH_CTL_CODE(47)
#define KPH_SSADDTHREADIDRULE KPH_CTL_CODE(48)
#define KPH_SSADDPREVIOUSMODERULE KPH_CTL_CODE(49)
#define KPH_SSADDNUMBERRULE KPH_CTL_CODE(50)
#define KPH_SSENABLECLIENTENTRY KPH_CTL_CODE(51)
#define KPH_OPENNAMEDOBJECT KPH_CTL_CODE(52)
#define KPH_QUERYINFORMATIONPROCESS KPH_CTL_CODE(53)
#define KPH_QUERYINFORMATIONTHREAD KPH_CTL_CODE(54)
#define KPH_SETINFORMATIONPROCESS KPH_CTL_CODE(55)
#define KPH_SETINFORMATIONTHREAD KPH_CTL_CODE(56)

// Process handle information

typedef struct _PROCESS_HANDLE
{
    HANDLE Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
    ULONG HandleAttributes;
} PROCESS_HANDLE, *PPROCESS_HANDLE;

typedef struct _PROCESS_HANDLE_INFORMATION
{
    ULONG HandleCount;
    PROCESS_HANDLE Handles[1];
} PROCESS_HANDLE_INFORMATION, *PPROCESS_HANDLE_INFORMATION;

// System service logging

typedef struct _KPHSS_CLIENT_INFORMATION
{
    HANDLE ProcessId;
    PVOID BufferBase;
    ULONG BufferSize;
    
    ULONG NumberOfBlocksWritten;
    ULONG NumberOfBlocksDropped;
} KPHSS_CLIENT_INFORMATION, *PKPHSS_CLIENT_INFORMATION;

// Driver information

typedef enum _DRIVER_INFORMATION_CLASS
{
    DriverBasicInformation,
    DriverNameInformation,
    DriverServiceKeyNameInformation,
    MaxDriverInfoClass
} DRIVER_INFORMATION_CLASS;

typedef struct _DRIVER_BASIC_INFORMATION
{
    ULONG Flags;
    PVOID DriverStart;
    ULONG DriverSize;
} DRIVER_BASIC_INFORMATION, *PDRIVER_BASIC_INFORMATION;

typedef struct _DRIVER_NAME_INFORMATION
{
    UNICODE_STRING DriverName;
} DRIVER_NAME_INFORMATION, *PDRIVER_NAME_INFORMATION;

typedef struct _DRIVER_SERVICE_KEY_NAME_INFORMATION
{
    UNICODE_STRING ServiceKeyName;
} DRIVER_SERVICE_KEY_NAME_INFORMATION, *PDRIVER_SERVICE_KEY_NAME_INFORMATION;

NTSTATUS KphInit();

NTSTATUS KphConnect(
    __out PHANDLE KphHandle,
    __in_opt PWSTR DeviceName
    );

NTSTATUS KphConnect2(
    __out PHANDLE KphHandle,
    __in_opt PWSTR DeviceName,
    __in PWSTR FileName
    );

NTSTATUS KphDisconnect(
    __in HANDLE KphHandle
    );

NTSTATUS KphGetFeatures(
    __in HANDLE KphHandle,
    __out PULONG Features
    );

NTSTATUS KphCloseHandle(
    __in HANDLE KphHandle,
    __in HANDLE Handle
    );

NTSTATUS KphSsQueryClientEntry(
    __in HANDLE KphHandle,
    __in HANDLE ClientEntryHandle,
    __out PKPHSS_CLIENT_INFORMATION ClientInformation,
    __in ULONG ClientInformationLength,
    __out PULONG ReturnLength
    );

NTSTATUS KphOpenProcess(
    __in HANDLE KphHandle,
    __out PHANDLE ProcessHandle,
    __in HANDLE ProcessId,
    __in ACCESS_MASK DesiredAccess
    );

NTSTATUS KphOpenThread(
    __in HANDLE KphHandle,
    __out PHANDLE ThreadHandle,
    __in HANDLE ThreadId,
    __in ACCESS_MASK DesiredAccess
    );

NTSTATUS KphOpenProcessToken(
    __in HANDLE KphHandle,
    __out PHANDLE TokenHandle,
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess
    );

NTSTATUS KphGetProcessProtected(
    __in HANDLE KphHandle,
    __in HANDLE ProcessId,
    __out PBOOLEAN IsProtected
    );

NTSTATUS KphSetProcessProtected(
    __in HANDLE KphHandle,
    __in HANDLE ProcessId,
    __in BOOLEAN IsProtected
    );

NTSTATUS KphTerminateProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    );

NTSTATUS KphSuspendProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    );

NTSTATUS KphResumeProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    );

NTSTATUS KphReadVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS KphWriteVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS KphSetProcessToken(
    __in HANDLE KphHandle,
    __in HANDLE SourceProcessId,
    __in HANDLE TargetProcessId
    );

NTSTATUS KphGetThreadStartAddress(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __out PPVOID StartAddress
    );

NTSTATUS KphSetHandleAttributes(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG Flags
    );

NTSTATUS KphGetHandleObjectName(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out_bcount_opt(BufferLength) PUNICODE_STRING Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS KphOpenProcessJob(
    __in HANDLE KphHandle,
    __out PHANDLE JobHandle,
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess
    );

NTSTATUS KphGetContextThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __inout PCONTEXT ThreadContext
    );

NTSTATUS KphSetContextThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in PCONTEXT ThreadContext
    );

NTSTATUS KphGetThreadWin32Thread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __out PPVOID Win32Thread
    );

NTSTATUS KphDuplicateObject(
    __in HANDLE KphHandle,
    __in HANDLE SourceProcessHandle,
    __in HANDLE SourceHandle,
    __in_opt HANDLE TargetProcessHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options
    );

NTSTATUS KphZwQueryObject(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __in_bcount_opt(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS KphGetProcessId(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out PHANDLE ProcessId
    );

NTSTATUS KphGetThreadId(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out PHANDLE ThreadId,
    __out_opt PHANDLE ProcessId
    );

NTSTATUS KphTerminateThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

NTSTATUS KphSetHandleGrantedAccess(
    __in HANDLE KphHandle,
    __in HANDLE Handle,
    __in ACCESS_MASK GrantedAccess
    );

NTSTATUS KphAssignImpersonationToken(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in HANDLE TokenHandle
    );

NTSTATUS KphProtectAdd(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in BOOLEAN AllowKernelMode,
    __in ACCESS_MASK ProcessAllowMask,
    __in ACCESS_MASK ThreadAllowMask
    );

NTSTATUS KphProtectRemove(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    );

NTSTATUS KphProtectQuery(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __out PBOOLEAN AllowKernelMode,
    __out PACCESS_MASK ProcessAllowMask,
    __out PACCESS_MASK ThreadAllowMask
    );

NTSTATUS KphUnsafeReadVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS KphSetExecuteOptions(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in ULONG ExecuteOptions
    );

NTSTATUS KphQueryProcessHandles(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __out_bcount_opt(BufferLength) PVOID Buffer,
    __in_opt ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS KphOpenThreadProcess(
    __in HANDLE KphHandle,
    __out PHANDLE ProcessHandle,
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess
    );

NTSTATUS KphCaptureStackBackTraceThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PPVOID BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash
    );

NTSTATUS KphDangerousTerminateThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

NTSTATUS KphOpenType(
    __in HANDLE KphHandle,
    __out PHANDLE TypeHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSTATUS KphOpenDriver(
    __in HANDLE KphHandle,
    __out PHANDLE DriverHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSTATUS KphQueryInformationDriver(
    __in HANDLE KphHandle,
    __in HANDLE DriverHandle,
    __in DRIVER_INFORMATION_CLASS DriverInformationClass,
    __out_bcount_opt(DriverInformationLength) PVOID DriverInformation,
    __in ULONG DriverInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS KphOpenDirectoryObject(
    __in HANDLE KphHandle,
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSTATUS KphOpenNamedObject(
    __in HANDLE KphHandle,
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSTATUS KphQueryInformationProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out_bcount_opt(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS KphQueryInformationThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in THREAD_INFORMATION_CLASS ThreadInformationClass,
    __out_bcount_opt(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS KphSetInformationProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __in_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength
    );

NTSTATUS KphSetInformationThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in THREAD_INFORMATION_CLASS ThreadInformationClass,
    __in_bcount(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength
    );

#endif

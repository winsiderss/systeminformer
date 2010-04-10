#ifndef KPH_H
#define KPH_H

#define KPH_SHORT_DEVICE_NAME (L"KProcessHacker2")

#define KPH_DEVICE_TYPE (0x9999)
#define KPH_DEVICE_NAME (L"\\Device\\KProcessHacker2")

#define KPHF_PSTERMINATEPROCESS 0x1
#define KPHF_PSPTERMINATETHREADBPYPOINTER 0x2

#define KPH_CTL_CODE(x) CTL_CODE(KPH_DEVICE_TYPE, 0x800 + x, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* General */
#define KPH_GETFEATURES KPH_CTL_CODE(0)

/* Processes */
#define KPH_OPENPROCESS KPH_CTL_CODE(50)
#define KPH_OPENPROCESSTOKEN KPH_CTL_CODE(51)
#define KPH_OPENPROCESSJOB KPH_CTL_CODE(52)
#define KPH_SUSPENDPROCESS KPH_CTL_CODE(53)
#define KPH_RESUMEPROCESS KPH_CTL_CODE(54)
#define KPH_TERMINATEPROCESS KPH_CTL_CODE(55)
#define KPH_READVIRTUALMEMORY KPH_CTL_CODE(56)
#define KPH_WRITEVIRTUALMEMORY KPH_CTL_CODE(57)
#define KPH_UNSAFEREADVIRTUALMEMORY KPH_CTL_CODE(58)
#define KPH_GETPROCESSPROTECTED KPH_CTL_CODE(59)
#define KPH_SETPROCESSPROTECTED KPH_CTL_CODE(60)
#define KPH_SETEXECUTEOPTIONS KPH_CTL_CODE(61)
#define KPH_SETPROCESSTOKEN KPH_CTL_CODE(62)
#define KPH_QUERYINFORMATIONPROCESS KPH_CTL_CODE(63)
#define KPH_QUERYINFORMATIONTHREAD KPH_CTL_CODE(64)
#define KPH_SETINFORMATIONPROCESS KPH_CTL_CODE(65)
#define KPH_SETINFORMATIONTHREAD KPH_CTL_CODE(66)

/* Threads */
#define KPH_OPENTHREAD KPH_CTL_CODE(100)
#define KPH_OPENTHREADPROCESS KPH_CTL_CODE(101)
#define KPH_TERMINATETHREAD KPH_CTL_CODE(102)
#define KPH_DANGEROUSTERMINATETHREAD KPH_CTL_CODE(103)
#define KPH_GETCONTEXTTHREAD KPH_CTL_CODE(104)
#define KPH_SETCONTEXTTHREAD KPH_CTL_CODE(105)
#define KPH_CAPTURESTACKBACKTRACETHREAD KPH_CTL_CODE(106)
#define KPH_GETTHREADWIN32THREAD KPH_CTL_CODE(107)
#define KPH_ASSIGNIMPERSONATIONTOKEN KPH_CTL_CODE(108)

/* Handles */
#define KPH_QUERYPROCESSHANDLES KPH_CTL_CODE(150)
#define KPH_GETHANDLEOBJECTNAME KPH_CTL_CODE(151)
#define KPH_ZWQUERYOBJECT KPH_CTL_CODE(152)
#define KPH_DUPLICATEOBJECT KPH_CTL_CODE(153)
#define KPH_SETHANDLEATTRIBUTES KPH_CTL_CODE(154)
#define KPH_SETHANDLEGRANTEDACCESS KPH_CTL_CODE(155)
#define KPH_GETPROCESSID KPH_CTL_CODE(156)
#define KPH_GETTHREADID KPH_CTL_CODE(157)

/* Objects */
#define KPH_OPENNAMEDOBJECT KPH_CTL_CODE(200)
#define KPH_OPENDIRECTORYOBJECT KPH_CTL_CODE(201)
#define KPH_OPENDRIVER KPH_CTL_CODE(202)
#define KPH_QUERYINFORMATIONDRIVER KPH_CTL_CODE(203)
#define KPH_OPENTYPE KPH_CTL_CODE(204)

// Process handle information

typedef struct _PROCESS_HANDLE
{
    HANDLE Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
    USHORT ObjectTypeIndex;
    USHORT Reserved1;
    ULONG HandleAttributes;
    ULONG Reserved2;
} PROCESS_HANDLE, *PPROCESS_HANDLE;

typedef struct _PROCESS_HANDLE_INFORMATION
{
    ULONG HandleCount;
    PROCESS_HANDLE Handles[1];
} PROCESS_HANDLE_INFORMATION, *PPROCESS_HANDLE_INFORMATION;

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

NTSTATUS KphInstall(
    __in_opt PWSTR DeviceName,
    __in PWSTR FileName
    );

NTSTATUS KphUninstall(
    __in_opt PWSTR DeviceName
    );

NTSTATUS KphGetFeatures(
    __in HANDLE KphHandle,
    __out PULONG Features
    );

NTSTATUS KphOpenProcess(
    __in HANDLE KphHandle,
    __out PHANDLE ProcessHandle,
    __in HANDLE ProcessId,
    __in ACCESS_MASK DesiredAccess
    );

NTSTATUS KphOpenProcessToken(
    __in HANDLE KphHandle,
    __out PHANDLE TokenHandle,
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess
    );

NTSTATUS KphOpenProcessJob(
    __in HANDLE KphHandle,
    __out PHANDLE JobHandle,
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess
    );

NTSTATUS KphSuspendProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    );

NTSTATUS KphResumeProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle
    );

NTSTATUS KphTerminateProcess(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
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

NTSTATUS KphUnsafeReadVirtualMemory(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
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

NTSTATUS KphSetExecuteOptions(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in ULONG ExecuteOptions
    );

NTSTATUS KphSetProcessToken(
    __in HANDLE KphHandle,
    __in HANDLE SourceProcessId,
    __in HANDLE TargetProcessId
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

NTSTATUS KphOpenThread(
    __in HANDLE KphHandle,
    __out PHANDLE ThreadHandle,
    __in HANDLE ThreadId,
    __in ACCESS_MASK DesiredAccess
    );

NTSTATUS KphOpenThreadProcess(
    __in HANDLE KphHandle,
    __out PHANDLE ProcessHandle,
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess
    );

NTSTATUS KphTerminateThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

NTSTATUS KphDangerousTerminateThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
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

NTSTATUS KphCaptureStackBackTraceThread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in ULONG FramesToSkip,
    __in ULONG FramesToCapture,
    __out_ecount(FramesToCapture) PPVOID BackTrace,
    __out_opt PULONG CapturedFrames,
    __out_opt PULONG BackTraceHash
    );

NTSTATUS KphGetThreadWin32Thread(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __out PPVOID Win32Thread
    );

NTSTATUS KphAssignImpersonationToken(
    __in HANDLE KphHandle,
    __in HANDLE ThreadHandle,
    __in HANDLE TokenHandle
    );

NTSTATUS KphQueryProcessHandles(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __out_bcount_opt(BufferLength) PVOID Buffer,
    __in_opt ULONG BufferLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS KphGetHandleObjectName(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __out_bcount_opt(BufferLength) PUNICODE_STRING Buffer,
    __in ULONG BufferLength,
    __out_opt PULONG ReturnLength
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

NTSTATUS KphSetHandleAttributes(
    __in HANDLE KphHandle,
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in ULONG Flags
    );

NTSTATUS KphSetHandleGrantedAccess(
    __in HANDLE KphHandle,
    __in HANDLE Handle,
    __in ACCESS_MASK GrantedAccess
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

NTSTATUS KphOpenNamedObject(
    __in HANDLE KphHandle,
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSTATUS KphOpenDirectoryObject(
    __in HANDLE KphHandle,
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
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

NTSTATUS KphOpenType(
    __in HANDLE KphHandle,
    __out PHANDLE TypeHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

#endif

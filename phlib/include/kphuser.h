#ifndef _PH_KPHUSER_H
#define _PH_KPHUSER_H

#include <kphapi.h>
#include <kphcomms.h>

EXTERN_C_START

#define KPH_SERVICE_NAME __TEXT("KSystemInformer")
#define KPH_OBJECT_NAME __TEXT("\\Driver\\KSystemInformer")
#define KPH_PORT_NAME __TEXT("\\KSystemInformer")
#define KPH_ALTITUDE_NAME __TEXT("385400")

#ifdef DEBUG
#define KSI_COMMS_INIT_ASSERT() assert(KphCommsIsConnected())
#else
#define KSI_COMMS_INIT_ASSERT()
#endif

typedef struct _KPH_CONFIG_PARAMETERS
{
    _In_ PPH_STRINGREF FileName;
    _In_ PPH_STRINGREF ServiceName;
    _In_ PPH_STRINGREF ObjectName;
    _In_ PPH_STRINGREF PortName;
    _In_ PPH_STRINGREF Altitude;
    _In_ BOOLEAN DisableImageLoadProtection;
    _In_opt_ PKPH_COMMS_CALLBACK Callback;
} KPH_CONFIG_PARAMETERS, *PKPH_CONFIG_PARAMETERS;

PHLIBAPI
NTSTATUS
NTAPI
KphConnect(
    _In_ PKPH_CONFIG_PARAMETERS Options
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetParameters(
    _In_ PKPH_CONFIG_PARAMETERS Options
    );

PHLIBAPI
VOID
NTAPI
KphSetServiceSecurity(
    _In_ SC_HANDLE ServiceHandle
    );

//PHLIBAPI
//NTSTATUS
//NTAPI
//KphInstall(
//    _In_ PPH_STRINGREF ServiceName,
//    _In_ PPH_STRINGREF ObjectName,
//    _In_ PPH_STRINGREF PortName,
//    _In_ PPH_STRINGREF FileName,
//    _In_ PPH_STRINGREF Altitude,
//    _In_ BOOLEAN DisableImageLoadProtection
//    );
//
//PHLIBAPI
//NTSTATUS
//NTAPI
//KphUninstall(
//    _In_ PPH_STRINGREF ServiceName
//    );

PHLIBAPI
NTSTATUS
NTAPI
KphServiceStop(
    _In_ PPH_STRINGREF ServiceName
    );

PHLIBAPI
PPH_FREE_LIST
NTAPI
KphGetMessageFreeList(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
KphGetInformerSettings(
    _Out_ PKPH_INFORMER_SETTINGS Settings
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformerSettings(
    _In_ PKPH_INFORMER_SETTINGS Settings
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenProcessJob(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE JobHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    );

PHLIBAPI
NTSTATUS
NTAPI
KphReadVirtualMemoryUnsafe(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Inout_opt_ PSIZE_T NumberOfBytesRead
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenThreadProcess(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE ProcessHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphCaptureStackBackTraceThread(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID *BackTrace,
    _Inout_opt_ PULONG CapturedFrames,
    _Inout_opt_ PULONG BackTraceHash
    );

PHLIBAPI
NTSTATUS
NTAPI
KphEnumerateProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_opt_ ULONG BufferLength,
    _Inout_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphEnumerateProcessHandles2(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_PROCESS_HANDLE_INFORMATION *Handles
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _In_reads_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationDriver(
    _In_ HANDLE DriverHandle,
    _In_ DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_writes_bytes_opt_(DriverInformationLength) PVOID DriverInformation,
    _In_ ULONG DriverInformationLength,
    _Inout_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _Out_writes_bytes_opt_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Inout_opt_ PULONG ReturnLength
    );

PHLIBAPI
KPH_PROCESS_STATE
NTAPI
KphGetProcessState(
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
KPH_PROCESS_STATE
NTAPI
KphGetCurrentProcessState(
    VOID
    );

typedef enum _KPH_LEVEL
{
    KphLevelNone,
    KphLevelMin,
    KphLevelLow,
    KphLevelMed,
    KphLevelHigh,
    KphLevelMax

} KPH_LEVEL;

PHLIBAPI
KPH_LEVEL
NTAPI
KphProcessLevel(
    _In_ HANDLE ProcessHandle
    );

PHLIBAPI
KPH_LEVEL
NTAPI
KphLevel(
    VOID
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSystemControl(
    _In_ KPH_SYSTEM_CONTROL_CLASS SystemControlClass,
    _In_reads_bytes_(SystemControlInfoLength) PVOID SystemControlInfo,
    _In_ ULONG SystemControlInfoLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphAlpcQueryInformation(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE PortHandle,
    _In_ KPH_ALPC_INFORMATION_CLASS AlpcInformationClass,
    _Out_writes_bytes_opt_(AlpcInformationLength) PVOID AlpcInformation,
    _In_ ULONG AlpcInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphAlpcQueryComminicationsNamesInfo(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE PortHandle,
    _Out_ PKPH_ALPC_COMMUNICATION_NAMES_INFORMATION* Names
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationFile(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_writes_bytes_(FileInformationLength) PVOID FileInformation,
    _In_ ULONG FileInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryVolumeInformationFile(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _In_ FS_INFORMATION_CLASS FsInformationClass,
    _Out_writes_bytes_(FsInformationLength) PVOID FsInformation,
    _In_ ULONG FsInformationLength,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
    );

PHLIBAPI
NTSTATUS
NTAPI
KphDuplicateObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE SourceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TargetHandle
    );

EXTERN_C_END

#endif

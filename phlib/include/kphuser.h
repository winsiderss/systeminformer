/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *     jxy-s   2021-2022
 *
 */

#ifndef _PH_KPHUSER_H
#define _PH_KPHUSER_H

#include <kphapi.h>
#include <kphcomms.h>

EXTERN_C_START

#define KPH_SERVICE_NAME TEXT("KSystemInformer")
#define KPH_OBJECT_NAME  TEXT("\\Driver\\KSystemInformer")
#define KPH_PORT_NAME    TEXT("\\KSystemInformer")

#ifdef DEBUG
#define KSI_COMMS_INIT_ASSERT() assert(KphMessageFreeList.Size == sizeof(KPH_MESSAGE))
#else
#define KSI_COMMS_INIT_ASSERT()
#endif

typedef struct _KPH_CONFIG_PARAMETERS
{
    _In_ PPH_STRINGREF FileName;
    _In_ PPH_STRINGREF ServiceName;
    _In_ PPH_STRINGREF ObjectName;
    _In_opt_ PPH_STRINGREF PortName;
    _In_opt_ PPH_STRINGREF Altitude;
    _In_ ULONG FsSupportedFeatures;
    _In_ KPH_PARAMETER_FLAGS Flags;
    _In_ BOOLEAN EnableNativeLoad;
    _In_ BOOLEAN EnableFilterLoad;
    _In_opt_ PKPH_COMMS_CALLBACK Callback;
} KPH_CONFIG_PARAMETERS, *PKPH_CONFIG_PARAMETERS;

PHLIBAPI
VOID
NTAPI
KphInitialize(
    VOID
    );

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

PHLIBAPI
NTSTATUS
NTAPI
KsiLoadUnloadService(
    _In_ PKPH_CONFIG_PARAMETERS Config,
    _In_ BOOLEAN LoadDriver
    );

PHLIBAPI
NTSTATUS
NTAPI
KphServiceStop(
    _In_ PKPH_CONFIG_PARAMETERS Config
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
KphReadVirtualMemory(
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

#define KPH_STACK_BACK_TRACE_USER_MODE   0x00000001ul
#define KPH_STACK_BACK_TRACE_SKIP_KPH    0x00000002ul
#define KPH_STACK_BACK_TRACE_NO_SENTINEL 0x00000004ul

PHLIBAPI
NTSTATUS
NTAPI
KphCaptureStackBackTraceThread(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID* BackTrace,
    _Out_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ ULONG Flags
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
KsiEnumerateProcessHandles(
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
KphQueryObjectSectionMappingsInfo(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _Out_ PKPH_SECTION_MAPPINGS_INFORMATION* Info
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
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationDriver(
    _In_ HANDLE DriverHandle,
    _In_ KPH_DRIVER_INFORMATION_CLASS DriverInformationClass,
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
KphLevelEx(
    _In_ BOOLEAN Cached
    );

PHLIBAPI
KPH_LEVEL
NTAPI
KsiLevel(
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

PHLIBAPI
NTSTATUS
NTAPI
KphQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter,
    _Out_opt_ PLARGE_INTEGER PerformanceFrequency
    );

#define IO_OPEN_TARGET_DIRECTORY        0x0004
#define IO_STOP_ON_SYMLINK              0x0008
#define IO_IGNORE_SHARE_ACCESS_CHECK    0x0800  // Ignores share access checks on opens.

PHLIBAPI
NTSTATUS
NTAPI
KphCreateFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Options
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ KPH_THREAD_INFORMATION_CLASS ThreadInformationClass,
    _Out_writes_bytes_opt_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQuerySection(
    _In_ HANDLE SectionHandle,
    _In_ KPH_SECTION_INFORMATION_CLASS SectionInformationClass,
    _Out_writes_bytes_(SectionInformationLength) PVOID SectionInformation,
    _In_ ULONG SectionInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQuerySectionMappingsInfo(
    _In_ HANDLE SectionHandle,
    _Out_ PKPH_SECTION_MAPPINGS_INFORMATION* Info
    );

PHLIBAPI
NTSTATUS
NTAPI
KphCompareObjects(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FirstObjectHandle,
    _In_ HANDLE SecondObjectHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphGetMessageTimeouts(
    _Out_ PKPH_MESSAGE_TIMEOUTS Timeouts
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetMessageTimeouts(
    _In_ PKPH_MESSAGE_TIMEOUTS Timeouts
    );

PHLIBAPI
NTSTATUS
NTAPI
KphAcquireDriverUnloadProtection(
    _Out_opt_ PLONG PreviousCount,
    _Out_opt_ PLONG ClientPreviousCount
    );

PHLIBAPI
NTSTATUS
NTAPI
KphReleaseDriverUnloadProtection(
    _Out_opt_ PLONG PreviousCount,
    _Out_opt_ PLONG ClientPreviousCount
    );

PHLIBAPI
NTSTATUS
NTAPI
KphGetConnectedClientCount(
    _Out_ PULONG Count
    );

PHLIBAPI
NTSTATUS
NTAPI
KphActivateDynData(
    _In_ PBYTE DynData,
    _In_ ULONG DynDataLength,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphRequestSessionAccessToken(
    _Out_ PKPH_SESSION_ACCESS_TOKEN AccessToken,
    _In_ PLARGE_INTEGER Expiry,
    _In_ ULONG Privileges,
    _In_ LONG Uses
    );

PHLIBAPI
NTSTATUS
NTAPI
KphAssignProcessSessionToken(
    _In_ HANDLE ProcessHandle,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphAssignThreadSessionToken(
    _In_ HANDLE ThreadHandle,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphGetInformerProcessFilter(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_INFORMER_SETTINGS Filter
    );

PHLIBAPI
NTSTATUS
NTAPI
KphSetInformerProcessFilter(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PKPH_INFORMER_SETTINGS Filter
    );

PHLIBAPI
NTSTATUS
NTAPI
KphStripProtectedProcessMasks(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK ProcessAllowedMask,
    _In_ ACCESS_MASK ThreadAllowedMask
    );

PHLIBAPI
NTSTATUS
NTAPI
KphQueryVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ KPH_MEMORY_INFORMATION_CLASS MemoryInformationClass,
    _Out_writes_bytes_opt_(MemoryInformationLength) PVOID MemoryInformation,
    _In_ ULONG MemoryInformationLength,
    _Out_opt_ PULONG ReturnLength
    );


PHLIBAPI
NTSTATUS
NTAPI
KsiQueryHashInformationFile(
    _In_ HANDLE FileHandle,
    _Inout_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG HashInformationLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenDevice(
    _Out_ PHANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenDeviceDriver(
    _In_ HANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE DriverHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
KphOpenDeviceBaseDevice(
    _In_ HANDLE DeviceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE BaseDeviceHandle
    );

EXTERN_C_END

#endif

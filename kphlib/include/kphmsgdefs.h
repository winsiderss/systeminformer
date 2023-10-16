/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#pragma once

#include <kphapi.h>

//
// PH -> KPH
//

#pragma warning(push)
#pragma warning(disable : 4201)

typedef struct _KPHM_SET_INFORMER_SETTINGS
{
    NTSTATUS Status;
    KPH_INFORMER_SETTINGS Settings;
} KPHM_SET_INFORMER_SETTINGS, *PKPHM_SET_INFORMER_SETTINGS;

typedef struct _KPHM_GET_INFORMER_SETTINGS
{
    NTSTATUS Status;
    KPH_INFORMER_SETTINGS Settings;
} KPHM_GET_INFORMER_SETTINGS, *PKPHM_GET_INFORMER_SETTINGS;

typedef struct _KPHM_OPEN_PROCESS
{
    NTSTATUS Status;
    PHANDLE ProcessHandle;
    ACCESS_MASK DesiredAccess;
    PCLIENT_ID ClientId;
} KPHM_OPEN_PROCESS, *PKPHM_OPEN_PROCESS;

typedef struct _KPHM_OPEN_PROCESS_TOKEN
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    ACCESS_MASK DesiredAccess;
    PHANDLE TokenHandle;
} KPHM_OPEN_PROCESS_TOKEN, *PKPHM_OPEN_PROCESS_TOKEN;

typedef struct _KPHM_OPEN_PROCESS_JOB
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    ACCESS_MASK DesiredAccess;
    PHANDLE JobHandle;
} KPHM_OPEN_PROCESS_JOB, *PKPHM_OPEN_PROCESS_JOB;

typedef struct _KPHM_TERMINATE_PROCESS
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    NTSTATUS ExitStatus;
} KPHM_TERMINATE_PROCESS, *PKPHM_TERMINATE_PROCESS;

typedef struct _KPHM_READ_VIRTUAL_MEMORY_UNSAFE
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    PVOID BaseAddress;
    PVOID Buffer;
    SIZE_T BufferSize;
    PSIZE_T NumberOfBytesRead;
} KPHM_READ_VIRTUAL_MEMORY_UNSAFE, *PKPHM_READ_VIRTUAL_MEMORY_UNSAFE;

typedef struct _KPHM_OPEN_THREAD
{
    NTSTATUS Status;
    PHANDLE ThreadHandle;
    ACCESS_MASK DesiredAccess;
    PCLIENT_ID ClientId;
} KPHM_OPEN_THREAD, *PKPHM_OPEN_THREAD;

typedef struct _KPHM_OPEN_THREAD_PROCESS
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    ACCESS_MASK DesiredAccess;
    PHANDLE ProcessHandle;
} KPHM_OPEN_THREAD_PROCESS, *PKPHM_OPEN_THREAD_PROCESS;

typedef struct _KPHM_CAPTURE_STACK_BACKTRACE_THREAD
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    ULONG FramesToSkip;
    ULONG FramesToCapture;
    PVOID* BackTrace;
    PULONG CapturedFrames;
    PULONG BackTraceHash;
    ULONG Flags;
    PLARGE_INTEGER Timeout;
} KPHM_CAPTURE_STACK_BACKTRACE_THREAD, *PKPHM_CAPTURE_STACK_BACKTRACE_THREAD;

typedef struct _KPHM_ENUMERATE_PROCESS_HANDLES
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    PVOID Buffer;
    ULONG BufferLength;
    PULONG ReturnLength;
} KPHM_ENUMERATE_PROCESS_HANDLES, *PKPHM_ENUMERATE_PROCESS_HANDLES;

typedef struct _KPHM_QUERY_INFORMATION_OBJECT
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    HANDLE Handle;
    KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass;
    PVOID ObjectInformation;
    ULONG ObjectInformationLength;
    PULONG ReturnLength;
} KPHM_QUERY_INFORMATION_OBJECT, *PKPHM_QUERY_INFORMATION_OBJECT;

typedef struct _KPHM_SET_INFORMATION_OBJECT
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    HANDLE Handle;
    KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass;
    PVOID ObjectInformation;
    ULONG ObjectInformationLength;
} KPHM_SET_INFORMATION_OBJECT, *PKPHM_SET_INFORMATION_OBJECT;

typedef struct _KPHM_OPEN_DRIVER
{
    NTSTATUS Status;
    PHANDLE DriverHandle;
    ACCESS_MASK DesiredAccess;
    POBJECT_ATTRIBUTES ObjectAttributes;
} KPHM_OPEN_DRIVER, *PKPHM_OPEN_DRIVER;

typedef struct _KPHM_QUERY_INFORMATION_DRIVER
{
    NTSTATUS Status;
    HANDLE DriverHandle;
    DRIVER_INFORMATION_CLASS DriverInformationClass;
    PVOID DriverInformation;
    ULONG DriverInformationLength;
    PULONG ReturnLength;
} KPHM_QUERY_INFORMATION_DRIVER, *PKPHM_QUERY_INFORMATION_DRIVER;

typedef struct _KPHM_QUERY_INFORMATION_PROCESS
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass;
    PVOID ProcessInformation;
    ULONG ProcessInformationLength;
    PULONG ReturnLength;
} KPHM_QUERY_INFORMATION_PROCESS, *PKPHM_QUERY_INFORMATION_PROCESS;

typedef struct _KPHM_SET_INFORMATION_PROCESS
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass;
    PVOID ProcessInformation;
    ULONG ProcessInformationLength;
} KPHM_SET_INFORMATION_PROCESS, *PKPHM_SET_INFORMATION_PROCESS;

typedef struct _KPHM_SET_INFORMATION_THREAD
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    KPH_THREAD_INFORMATION_CLASS ThreadInformationClass;
    PVOID ThreadInformation;
    ULONG ThreadInformationLength;
} KPHM_SET_INFORMATION_THREAD, *PKPHM_SET_INFORMATION_THREAD;

typedef struct _KPHM_SYSTEM_CONTROL
{
    NTSTATUS Status;
    KPH_SYSTEM_CONTROL_CLASS SystemControlClass;
    PVOID SystemControlInfo;
    ULONG SystemControlInfoLength;
} KPHM_SYSTEM_CONTROL, *PKPHM_SYSTEM_CONTROL;

typedef struct _KPHM_ALPC_QUERY_INFORMATION
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    HANDLE PortHandle;
    KPH_ALPC_INFORMATION_CLASS AlpcInformationClass;
    PVOID AlpcInformation;
    ULONG AlpcInformationLength;
    PULONG ReturnLength;
} KPHM_ALPC_QUERY_INFORMATION, *PKPHM_ALPC_QUERY_INFORMATION;

typedef struct _KPHM_QUERY_INFORMATION_FILE
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    HANDLE FileHandle;
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID FileInformation;
    ULONG FileInformationLength;
    PIO_STATUS_BLOCK IoStatusBlock;
} KPHM_QUERY_INFORMATION_FILE, *PKPHM_QUERY_INFORMATION_FILE;

typedef struct _KPHM_QUERY_VOLUME_INFORMATION_FILE
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    HANDLE FileHandle;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID FsInformation;
    ULONG FsInformationLength;
    PIO_STATUS_BLOCK IoStatusBlock;
} KPHM_QUERY_VOLUME_INFORMATION_FILE, *PKPHM_QUERY_VOLUME_INFORMATION_FILE;

typedef struct _KPHM_DUPLICATE_OBJECT
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    HANDLE SourceHandle;
    ACCESS_MASK DesiredAccess;
    PHANDLE TargetHandle;
} KPHM_DUPLICATE_OBJECT, *PKPHM_DUPLICATE_OBJECT;

typedef struct _KPHM_QUERY_PERFORMANCE_COUNTER
{
    LARGE_INTEGER PerformanceCounter;
    LARGE_INTEGER PerformanceFrequency;
} KPHM_QUERY_PERFORMANCE_COUNTER, *PKPHM_QUERY_PERFORMANCE_COUNTER;

typedef struct _KPHM_CREATE_FILE
{
    NTSTATUS Status;
    PHANDLE FileHandle;
    ACCESS_MASK DesiredAccess;
    POBJECT_ATTRIBUTES ObjectAttributes;
    PIO_STATUS_BLOCK IoStatusBlock;
    PLARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG CreateDisposition;
    ULONG CreateOptions;
    PVOID EaBuffer;
    ULONG EaLength;
    ULONG Options;
} KPHM_CREATE_FILE, *PKPHM_CREATE_FILE;

typedef struct _KPHM_QUERY_INFORMATION_THREAD
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    KPH_THREAD_INFORMATION_CLASS ThreadInformationClass;
    PVOID ThreadInformation;
    ULONG ThreadInformationLength;
    PULONG ReturnLength;
} KPHM_QUERY_INFORMATION_THREAD, *PKPHM_QUERY_INFORMATION_THREAD;

typedef struct _KPHM_QUERY_SECTION
{
    NTSTATUS Status;
    HANDLE SectionHandle;
    KPH_SECTION_INFORMATION_CLASS SectionInformationClass;
    PVOID SectionInformation;
    ULONG SectionInformationLength;
    PULONG ReturnLength;
} KPHM_QUERY_SECTION, *PKPHM_QUERY_SECTION;

typedef struct _KPHM_COMPARE_OBJECTS
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    HANDLE FirstObjectHandle;
    HANDLE SecondObjectHandle;
} KPHM_COMPARE_OBJECTS, *PKPHM_COMPARE_OBJECTS;

typedef struct _KPHM_GET_MESSAGE_TIMEOUTS
{
    KPH_MESSAGE_TIMEOUTS Timeouts;
} KPHM_GET_MESSAGE_TIMEOUTS, *PKPHM_GET_MESSAGE_TIMEOUTS;

typedef struct _KPHM_SET_MESSAGE_TIMEOUTS
{
    NTSTATUS Status;
    KPH_MESSAGE_TIMEOUTS Timeouts;
} KPHM_SET_MESSAGE_TIMEOUTS, *PKPHM_SET_MESSAGE_TIMEOUTS;

typedef struct _KPHM_ACQUIRE_DRIVER_UNLOAD_PROTECTION
{
    NTSTATUS Status;
    LONG PreviousCount;
    LONG ClientPreviousCount;
} KPHM_ACQUIRE_DRIVER_UNLOAD_PROTECTION, *PKPHM_ACQUIRE_DRIVER_UNLOAD_PROTECTION;

typedef struct _KPHM_RELEASE_DRIVER_UNLOAD_PROTECTION
{
    NTSTATUS Status;
    LONG PreviousCount;
    LONG ClientPreviousCount;
} KPHM_RELEASE_DRIVER_UNLOAD_PROTECTION, *PKPHM_RELEASE_DRIVER_UNLOAD_PROTECTION;

typedef struct _KPHM_GET_CONNECTED_CLIENT_COUNT
{
    ULONG Count;
} KPHM_GET_CONNECTED_CLIENT_COUNT, *PKPHM_GET_CONNECTED_CLIENT_COUNT;

//
// KPH -> PH
//

typedef struct _KPHM_PROCESS_CREATE
{
    CLIENT_ID CreatingClientId;
    HANDLE TargetProcessId;
    HANDLE ParentProcessId;
    BOOLEAN IsSubsystemProcess;

    //
    // Dynamic
    //
    // id: KphMsgFieldFileName       type: KphMsgTypeUnicodeString
    // id: KphMsgFieldCommandLine    type: KphMsgTypeUnicodeString
    //
} KPHM_PROCESS_CREATE, *PKPHM_PROCESS_CREATE;

typedef struct _KPHM_PROCESS_CREATE_REPLY
{
    NTSTATUS CreationStatus;
} KPHM_PROCESS_CREATE_REPLY, *PKPHM_PROCESS_CREATE_REPLY;

typedef struct _KPHM_PROCESS_EXIT
{
    CLIENT_ID ExitingClientId;
    NTSTATUS ExitStatus;
} KPHM_PROCESS_EXIT, *PKPHM_PROCESS_EXIT;

typedef struct _KPHM_THREAD_CREATE
{
    CLIENT_ID CreatingClientId;
    CLIENT_ID TargetClientId;
} KPHM_THREAD_CREATE, *PKPHM_THREAD_CREATE;

typedef struct _KPHM_THREAD_EXECUTE
{
    CLIENT_ID ExecutingClientId;
} KPHM_THREAD_EXECUTE, *PKPHM_THREAD_EXECUTE;

typedef struct _KPHM_THREAD_EXIT
{
    CLIENT_ID ExitingClientId;
    NTSTATUS ExitStatus;
} KPHM_THREAD_EXIT, *PKPHM_THREAD_EXIT;

typedef struct _KPHM_IMAGE_LOAD
{
    CLIENT_ID LoadingClientId;
    HANDLE TargetProcessId;

    union
    {
        ULONG Properties;
        struct
        {
            ULONG ImageAddressingMode : 8;
            ULONG SystemModeImage : 1;
            ULONG ImageMappedToAllPids : 1;
            ULONG ExtendedInfoPresent : 1;
            ULONG MachineTypeMismatch : 1;
            ULONG ImageSignatureLevel : 4;
            ULONG ImageSignatureType : 3;
            ULONG ImagePartialMap : 1;
            ULONG Reserved : 12;
        };
    };

    PVOID ImageBase;
    ULONG ImageSelector;
    SIZE_T ImageSize;
    ULONG ImageSectionNumber;

    //
    // Dynamic
    //
    // id: KphMsgFieldFileName    type: KphMsgTypeUnicodeString
    //
} KPHM_IMAGE_LOAD, *PKPHM_IMAGE_LOAD;

typedef struct _KPHM_DEBUG_PRINT
{
    CLIENT_ID ContextClientId;
    ULONG ComponentId;
    ULONG Level;

    //
    // Dynamic
    //
    // id: KphMsgFieldOutput      type: KphMsgTypeAnsiString
    //
} KPHM_DEBUG_PRINT, *PKPHM_DEBUG_PRINT;

typedef struct _KPHM_PROCESS_HANDLE_PRE_CREATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    ACCESS_MASK DesiredAccess;
    ACCESS_MASK OriginalDesiredAccess;

    HANDLE ObjectProcessId;
} KPHM_PROCESS_HANDLE_PRE_CREATE, *PKPHM_PROCESS_HANDLE_PRE_CREATE;

typedef struct _KPHM_PROCESS_HANDLE_POST_CREATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    NTSTATUS ReturnStatus;
    ACCESS_MASK GrantedAccess;

    HANDLE ObjectProcessId;
} KPHM_PROCESS_HANDLE_POST_CREATE, *PKPHM_PROCESS_HANDLE_POST_CREATE;

typedef struct _KPHM_PROCESS_HANDLE_PRE_DUPLICATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    ACCESS_MASK DesiredAccess;
    ACCESS_MASK OriginalDesiredAccess;

    HANDLE SourceProcessId;
    HANDLE TargetProcessId;

    HANDLE ObjectProcessId;
} KPHM_PROCESS_HANDLE_PRE_DUPLICATE, *PKPHM_PROCESS_HANDLE_PRE_DUPLICATE;

typedef struct _KPHM_PROCESS_HANDLE_POST_DUPLICATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    NTSTATUS ReturnStatus;
    ACCESS_MASK GrantedAccess;

    HANDLE ObjectProcessId;
} KPHM_PROCESS_HANDLE_POST_DUPLICATE, *PKPHM_PROCESS_HANDLE_POST_DUPLICATE;

typedef struct _KPHM_THREAD_HANDLE_PRE_CREATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    ACCESS_MASK DesiredAccess;
    ACCESS_MASK OriginalDesiredAccess;

    HANDLE ObjectThreadId;
} KPHM_THREAD_HANDLE_PRE_CREATE, *PKPHM_THREAD_HANDLE_PRE_CREATE;

typedef struct _KPHM_THREAD_HANDLE_POST_CREATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    NTSTATUS ReturnStatus;
    ACCESS_MASK GrantedAccess;

    HANDLE ObjectThreadId;
} KPHM_THREAD_HANDLE_POST_CREATE, *PKPHM_THREAD_HANDLE_POST_CREATE;

typedef struct _KPHM_THREAD_HANDLE_PRE_DUPLICATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    ACCESS_MASK DesiredAccess;
    ACCESS_MASK OriginalDesiredAccess;

    HANDLE SourceProcessId;
    HANDLE TargetProcessId;

    HANDLE ObjectThreadId;
} KPHM_THREAD_HANDLE_PRE_DUPLICATE, *PKPHM_THREAD_HANDLE_PRE_DUPLICATE;

typedef struct _KPHM_THREAD_HANDLE_POST_DUPLICATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    NTSTATUS ReturnStatus;
    ACCESS_MASK GrantedAccess;

    HANDLE ObjectThreadId;
} KPHM_THREAD_HANDLE_POST_DUPLICATE, *PKPHM_THREAD_HANDLE_POST_DUPLICATE;

typedef struct _KPHM_DESKTOP_HANDLE_PRE_CREATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    ACCESS_MASK DesiredAccess;
    ACCESS_MASK OriginalDesiredAccess;

    //
    // Dynamic
    //
    // id: KphMsgFieldObjectName    type: KphMsgTypeUnicodeString
    //
} KPHM_DESKTOP_HANDLE_PRE_CREATE, *PKPHM_DESKTOP_HANDLE_PRE_CREATE;

typedef struct _KPHM_DESKTOP_HANDLE_POST_CREATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    NTSTATUS ReturnStatus;
    ACCESS_MASK GrantedAccess;

    //
    // Dynamic
    //
    // id: KphMsgFieldObjectName    type: KphMsgTypeUnicodeString
    //
} KPHM_DESKTOP_HANDLE_POST_CREATE, *PKPHM_DESKTOP_HANDLE_POST_CREATE;

typedef struct _KPHM_DESKTOP_HANDLE_PRE_DUPLICATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    ACCESS_MASK DesiredAccess;
    ACCESS_MASK OriginalDesiredAccess;

    HANDLE SourceProcessId;
    HANDLE TargetProcessId;

    //
    // Dynamic
    //
    // id: KphMsgFieldObjectName    type: KphMsgTypeUnicodeString
    //
} KPHM_DESKTOP_HANDLE_PRE_DUPLICATE, *PKPHM_DESKTOP_HANDLE_PRE_DUPLICATE;

typedef struct _KPHM_DESKTOP_HANDLE_POST_DUPLICATE
{
    CLIENT_ID ContextClientId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    NTSTATUS ReturnStatus;
    ACCESS_MASK GrantedAccess;

    //
    // Dynamic
    //
    // id: KphMsgFieldObjectName    type: KphMsgTypeUnicodeString
    //
} KPHM_DESKTOP_HANDLE_POST_DUPLICATE, *PKPHM_DESKTOP_HANDLE_POST_DUPLICATE;

typedef struct _KPHM_REQUIRED_STATE_FAILURE
{
    CLIENT_ID ClientId;
    enum _KPH_MESSAGE_ID MessageId;
    KPH_PROCESS_STATE ClientState;
    KPH_PROCESS_STATE RequiredState;
} KPHM_REQUIRED_STATE_FAILURE, *PKPHM_REQUIRED_STATE_FAILURE;

#pragma warning(pop)

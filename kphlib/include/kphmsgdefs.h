/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
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
    PKPH_INFORMER_SETTINGS Settings;
} KPHM_SET_INFORMER_SETTINGS, *PKPHM_SET_INFORMER_SETTINGS;

typedef struct _KPHM_GET_INFORMER_SETTINGS
{
    NTSTATUS Status;
    PKPH_INFORMER_SETTINGS Settings;
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

typedef struct _KPHM_READ_VIRTUAL_MEMORY
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    PVOID BaseAddress;
    PVOID Buffer;
    SIZE_T BufferSize;
    PSIZE_T NumberOfBytesRead;
} KPHM_READ_VIRTUAL_MEMORY, *PKPHM_READ_VIRTUAL_MEMORY;

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
    KPH_DRIVER_INFORMATION_CLASS DriverInformationClass;
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

typedef struct _KPHM_GET_MESSAGE_SETTINGS
{
    NTSTATUS Status;
    PKPH_MESSAGE_SETTINGS Settings;
} KPHM_GET_MESSAGE_SETTINGS, *PKPHM_GET_MESSAGE_SETTINGS;

typedef struct _KPHM_SET_MESSAGE_SETTINGS
{
    NTSTATUS Status;
    PKPH_MESSAGE_SETTINGS Settings;
} KPHM_SET_MESSAGE_SETTINGS, *PKPHM_SET_MESSAGE_SETTINGS;

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

typedef struct _KPHM_ACTIVATE_DYNDATA
{
    NTSTATUS Status;
    PBYTE DynData;
    ULONG DynDataLength;
    PBYTE Signature;
    ULONG SignatureLength;
} KPHM_ACTIVATE_DYNDATA, *PKPHM_ACTIVATE_DYNDATA;

typedef struct _KPHM_REQUEST_SESSION_ACCESS_TOKEN
{
    NTSTATUS Status;
    KPH_SESSION_ACCESS_TOKEN AccessToken;
    LARGE_INTEGER Expiry;
    ULONG Privileges;
    LONG Uses;
} KPHM_REQUEST_SESSION_ACCESS_TOKEN, *PKPHM_REQUEST_SESSION_ACCESS_TOKEN;

typedef struct _KPHM_ASSIGN_PROCESS_SESSION_TOKEN
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    PBYTE Signature;
    ULONG SignatureLength;
} KPHM_ASSIGN_PROCESS_SESSION_TOKEN, *PKPHM_ASSIGN_PROCESS_SESSION_TOKEN;

typedef struct _KPHM_ASSIGN_THREAD_SESSION_TOKEN
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    PBYTE Signature;
    ULONG SignatureLength;
} KPHM_ASSIGN_THREAD_SESSION_TOKEN, *PKPHM_ASSIGN_THREAD_SESSION_TOKEN;

typedef struct _KPHM_GET_INFORMER_PROCESS_SETTINGS
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    PKPH_INFORMER_SETTINGS Settings;
} KPHM_GET_INFORMER_PROCESS_SETTINGS, *PKPHM_GET_INFORMER_PROCESS_SETTINGS;

typedef struct _KPHM_SET_INFORMER_PROCESS_SETTINGS
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    PKPH_INFORMER_SETTINGS Settings;
} KPHM_SET_INFORMER_PROCESS_SETTINGS, *PKPHM_SET_INFORMER_PROCESS_SETTINGS;

typedef struct _KPHM_STRIP_PROTECTED_PROCESS_MASKS
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    ACCESS_MASK ProcessAllowedMask;
    ACCESS_MASK ThreadAllowedMask;
} KPHM_STRIP_PROTECTED_PROCESS_MASKS, *PKPHM_STRIP_PROTECTED_PROCESS_MASKS;

typedef struct _KPHM_QUERY_VIRTUAL_MEMORY
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    PVOID BaseAddress;
    KPH_MEMORY_INFORMATION_CLASS MemoryInformationClass;
    PVOID MemoryInformation;
    ULONG MemoryInformationLength;
    PULONG ReturnLength;
} KPHM_QUERY_VIRTUAL_MEMORY, *PKPHM_QUERY_VIRTUAL_MEMORY;

typedef struct _KPHM_QUERY_HASH_INFORMATION_FILE
{
    NTSTATUS Status;
    HANDLE FileHandle;
    PKPH_HASH_INFORMATION HashingInformation;
    ULONG HashingInformationLength;
} KPHM_QUERY_HASH_INFORMATION_FILE, *PKPHM_QUERY_HASH_INFORMATION_FILE;

typedef struct _KPHM_OPEN_DEVICE
{
    NTSTATUS Status;
    PHANDLE DeviceHandle;
    ACCESS_MASK DesiredAccess;
    POBJECT_ATTRIBUTES ObjectAttributes;
} KPHM_OPEN_DEVICE, *PKPHM_OPEN_DEVICE;

typedef struct _KPHM_OPEN_DEVICE_DRIVER
{
    NTSTATUS Status;
    HANDLE DeviceHandle;
    ACCESS_MASK DesiredAccess;
    PHANDLE DriverHandle;
} KPHM_OPEN_DEVICE_DRIVER, *PKPHM_OPEN_DEVICE_DRIVER;

typedef struct _KPHM_OPEN_DEVICE_BASE_DEVICE
{
    NTSTATUS Status;
    HANDLE DeviceHandle;
    ACCESS_MASK DesiredAccess;
    PHANDLE BaseDeviceHandle;
} KPHM_OPEN_DEVICE_BASE_DEVICE, *PKPHM_OPEN_DEVICE_BASE_DEVICE;

typedef struct _KPHM_GET_INFORMER_STATS
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    PKPH_INFORMER_STATS Stats;
} KPHM_GET_INFORMER_STATS, *PKPHM_GET_INFORMER_STATS;

typedef struct _KPHM_GET_MESSAGE_STATS
{
    NTSTATUS Status;
    PKPH_INFORMER_STATS Stats;
} KPHM_GET_MESSAGE_STATS, *PKPHM_GET_MESSAGE_STATS;

//
// KPH -> PH
//

typedef struct _KPHM_PROCESS_CREATE
{
    CLIENT_ID CreatingClientId;
    ULONG64 CreatingProcessStartKey;
    PVOID CreatingThreadSubProcessTag;
    HANDLE TargetProcessId;
    ULONG64 TargetProcessStartKey;
    HANDLE ParentProcessId;
    ULONG64 ParentProcessStartKey;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG FileOpenNameAvailable : 1;
            ULONG IsSubsystemProcess : 1;
            ULONG Reserved : 30;
        };
    };

    PVOID FileObject;

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
    CLIENT_ID ClientId;
    ULONG64 ProcessStartKey;
    PVOID ThreadSubProcessTag;
    NTSTATUS ExitStatus;
} KPHM_PROCESS_EXIT, *PKPHM_PROCESS_EXIT;

typedef struct _KPHM_THREAD_CREATE
{
    CLIENT_ID CreatingClientId;
    ULONG64 CreatingProcessStartKey;
    PVOID CreatingThreadSubProcessTag;
    CLIENT_ID TargetClientId;
    ULONG64 TargetProcessStartKey;
} KPHM_THREAD_CREATE, *PKPHM_THREAD_CREATE;

typedef struct _KPHM_THREAD_EXECUTE
{
    CLIENT_ID ClientId;
    ULONG64 ProcessStartKey;
    PVOID ThreadSubProcessTag;
} KPHM_THREAD_EXECUTE, *PKPHM_THREAD_EXECUTE;

typedef struct _KPHM_THREAD_EXIT
{
    CLIENT_ID ClientId;
    ULONG64 ProcessStartKey;
    PVOID ThreadSubProcessTag;
    NTSTATUS ExitStatus;
} KPHM_THREAD_EXIT, *PKPHM_THREAD_EXIT;

typedef struct _KPHM_IMAGE_LOAD
{
    CLIENT_ID LoadingClientId;
    ULONG64 LoadingProcessStartKey;
    PVOID LoadingThreadSubProcessTag;
    HANDLE TargetProcessId;
    ULONG64 TargetProcessStartKey;

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
    PVOID FileObject;

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

typedef struct _KPHM_HANDLE
{
    CLIENT_ID ContextClientId;
    ULONG64 ContextProcessStartKey;
    PVOID ContextThreadSubProcessTag;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG KernelHandle : 1;
            ULONG Reserved : 31;
        };
    };

    PVOID Object;

    union
    {
        USHORT Information;
        struct
        {
            USHORT PostOperation : 1;
            USHORT Duplicate : 1;      // Create = 0, Duplicate = 1
            USHORT Spare : 14;
        };
    };

    ULONG64 Sequence;

    union
    {
        struct
        {
            ACCESS_MASK DesiredAccess;
            ACCESS_MASK OriginalDesiredAccess;

            union
            {
                struct
                {
                    union
                    {
                        struct
                        {
                            HANDLE ProcessId;
                        } Process; // KphMsgHandlePreCreateProcess

                        struct
                        {
                            CLIENT_ID ClientId;
                            PVOID SubProcessTag;
                        } Thread; // KphMsgHandlePreCreateThread
                    };
                } Create;

                struct
                {
                    HANDLE SourceProcessId;
                    HANDLE TargetProcessId;

                    union
                    {
                        struct
                        {
                            HANDLE ProcessId;
                        } Process; // KphMsgHandlePreDuplicateProcess

                        struct
                        {
                            CLIENT_ID ClientId;
                            PVOID SubProcessTag;
                        } Thread; // KphMsgHandlePreDuplicateThread
                    };
                } Duplicate;
            };
        } Pre;

        struct
        {
            ULONG64 PreSequence;
            LARGE_INTEGER PreTimeStamp;
            NTSTATUS ReturnStatus;
            ACCESS_MASK GrantedAccess;
            ACCESS_MASK DesiredAccess;
            ACCESS_MASK OriginalDesiredAccess;

            union
            {
                struct
                {
                    union
                    {
                        struct
                        {
                            HANDLE ProcessId;
                        } Process; // KphMsgHandlePostCreateProcess

                        struct
                        {
                            CLIENT_ID ClientId;
                            PVOID SubProcessTag;
                        } Thread; // KphMsgHandlePostCreateThread
                    };
                } Create;

                struct
                {
                    HANDLE SourceProcessId;
                    HANDLE TargetProcessId;

                    union
                    {
                        struct
                        {
                            HANDLE ProcessId;
                        } Process; // KphMsgHandlePostDuplicateProcess

                        struct
                        {
                            CLIENT_ID ClientId;
                            PVOID SubProcessTag;
                        } Thread; // KphMsgHandlePostDuplicateThread
                    };
                } Duplicate;
            };
        } Post;
    };

    //
    // Dynamic
    //
    // id: KphMsgFieldObjectName    type: KphMsgTypeUnicodeString
    //     - KphMsgHandlePreCreateDesktop
    //     - KphMsgHandlePostCreateDesktop
    //     - KphMsgHandlePreDuplicateDesktop
    //     - KphMsgHandlePostDuplicateDesktop
    //
} KPHM_HANDLE, *PKPHM_HANDLE;

typedef struct _KPHM_REQUIRED_STATE_FAILURE
{
    CLIENT_ID ClientId;
    enum _KPH_MESSAGE_ID MessageId;
    KPH_PROCESS_STATE ClientState;
    KPH_PROCESS_STATE RequiredState;
} KPHM_REQUIRED_STATE_FAILURE, *PKPHM_REQUIRED_STATE_FAILURE;

//
// FLT_PARAMETERS adapted for KPH_MESSAGE, there are some minor renaming of
// members to fit with KPH_MESSAGE convention.
//
#pragma warning(push)
#pragma warning(disable : 4324) // structure was padded due to alignment specifier
typedef union _KPHM_FILE_PARAMETERS
{
    // FLT_PARAMETERS.Create
    struct
    {
        PVOID SecurityContext; //PIO_SECURITY_CONTEXT
        ULONG Options;
        USHORT POINTER_ALIGNMENT FileAttributes;
        USHORT ShareAccess;
        ULONG POINTER_ALIGNMENT EaLength;
        PVOID EaBuffer;
        LARGE_INTEGER AllocationSize;
    } Create;

    // FLT_PARAMETERS.CreatePipe
    struct
    {
        PVOID SecurityContext; //PIO_SECURITY_CONTEXT
        ULONG Options;
        USHORT POINTER_ALIGNMENT Reserved;
        USHORT ShareAccess;
        PVOID Parameters;
    } CreateNamedPipe;

    // FLT_PARAMETERS.CreateMailslot
    struct
    {
        PVOID SecurityContext; // PIO_SECURITY_CONTEXT
        ULONG Options;
        USHORT POINTER_ALIGNMENT Reserved;
        USHORT ShareAccess;
        PVOID Parameters;
    } CreateMailslot;

    // FLT_PARAMETERS.Read
    struct
    {
        ULONG Length;
        ULONG POINTER_ALIGNMENT Key;
        LARGE_INTEGER ByteOffset;
        PVOID ReadBuffer;
        PVOID MdlAddress; // PMDL
    } Read;

    // FLT_PARAMETERS.Write
    struct
    {
        ULONG Length;
        ULONG POINTER_ALIGNMENT Key;
        LARGE_INTEGER ByteOffset;
        PVOID WriteBuffer;
        PVOID MdlAddress; // PMDL
    } Write;

    // FLT_PARAMETERS.QueryFileInformation
    struct
    {
        ULONG Length;
        FILE_INFORMATION_CLASS POINTER_ALIGNMENT FileInformationClass;
        PVOID InfoBuffer;
    } QueryInformation;

    // FLT_PARAMETERS.SetFileInformation
    struct
    {
        ULONG Length;
        FILE_INFORMATION_CLASS POINTER_ALIGNMENT FileInformationClass;
        PFILE_OBJECT ParentOfTarget;
        union
        {
            struct
            {
                BOOLEAN ReplaceIfExists;
                BOOLEAN AdvanceOnly;
            };
            ULONG ClusterCount;
            HANDLE DeleteHandle;
        };
        PVOID InfoBuffer;
    } SetInformation;

    // FLT_PARAMETERS.QueryEa
    struct
    {
        ULONG Length;
        PVOID EaList;
        ULONG EaListLength;
        ULONG POINTER_ALIGNMENT EaIndex;
        PVOID EaBuffer;
        PVOID MdlAddress; // PMDL
    } QueryEa;

    // FLT_PARAMETERS.SetEa
    struct
    {
        ULONG Length;
        PVOID EaBuffer;
        PVOID MdlAddress; // PMDL
    } SetEa;

    // FLT_PARAMETERS.QueryVolumeInformation
    struct
    {
        ULONG Length;
        FS_INFORMATION_CLASS POINTER_ALIGNMENT FsInformationClass;
        PVOID VolumeBuffer;
    } QueryVolumeInformation;

    // FLT_PARAMETERS.SetVolumeInformation
    struct
    {
        ULONG Length;
        FS_INFORMATION_CLASS POINTER_ALIGNMENT FsInformationClass;
        PVOID VolumeBuffer;
    } SetVolumeInformation;

    // FLT_PARAMETERS.DirectoryControl
    union
    {
        struct
        {
            ULONG Length;
            PUNICODE_STRING FileName;
            FILE_INFORMATION_CLASS FileInformationClass;
            ULONG POINTER_ALIGNMENT FileIndex;
            PVOID DirectoryBuffer;
            PVOID MdlAddress; // PMDL
        } QueryDirectory;

        struct
        {
            ULONG Length;
            ULONG POINTER_ALIGNMENT CompletionFilter;
            ULONG POINTER_ALIGNMENT Spare1;
            ULONG POINTER_ALIGNMENT Spare2;
            PVOID DirectoryBuffer;
            PVOID MdlAddress; // PMDL
        } NotifyDirectory;

        struct
        {
            ULONG Length;
            ULONG POINTER_ALIGNMENT CompletionFilter;
            DIRECTORY_NOTIFY_INFORMATION_CLASS POINTER_ALIGNMENT DirectoryNotifyInformationClass;
            ULONG POINTER_ALIGNMENT Spare2;
            PVOID DirectoryBuffer;
            PVOID MdlAddress; // PMDL
        } NotifyDirectoryEx;
    } DirectoryControl;

    // FLT_PARAMETERS.FileSystemControl
    union
    {
        struct
        {
            PVOID Vpb; // PVPB
            PDEVICE_OBJECT DeviceObject;
        } VerifyVolume;

        struct
        {
            ULONG OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT FsControlCode;
        } Common;

        struct
        {
            ULONG OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT FsControlCode;
            PVOID InputBuffer;
            PVOID OutputBuffer;
            PVOID OutputMdlAddress; // PMDL
        } Neither;

        struct
        {
            ULONG OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT FsControlCode;
            PVOID SystemBuffer;
        } Buffered;

        struct
        {
            ULONG OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT FsControlCode;
            PVOID InputSystemBuffer;
            PVOID OutputBuffer;
            PVOID OutputMdlAddress; // PMDL
        } Direct;
    } FileSystemControl;

    // FLT_PARAMETERS.DeviceIoControl
    union
    {
        struct
        {
            ULONG OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
        } Common;

        struct
        {
            ULONG OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
            PVOID InputBuffer;
            PVOID OutputBuffer;
            PVOID OutputMdlAddress; // PMDL
        } Neither;

        struct
        {
            ULONG OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
            PVOID SystemBuffer;
        } Buffered;

        struct
        {
            ULONG OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
            PVOID InputSystemBuffer;
            PVOID OutputBuffer;
            PVOID OutputMdlAddress; // PMDL
        } Direct;

        struct
        {
            ULONG OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
            PVOID InputBuffer;
            PVOID OutputBuffer;
        } FastIo;
    } DeviceControl;

    // FLT_PARAMETERS.LockControl
    struct
    {
        PLARGE_INTEGER Length;
        ULONG POINTER_ALIGNMENT Key;
        LARGE_INTEGER ByteOffset;
        PVOID ProcessId; // PEPROCESS
        BOOLEAN FailImmediately;
        BOOLEAN ExclusiveLock;
    } LockControl;

    // FLT_PARAMETERS.QuerySecurity
    struct
    {
        SECURITY_INFORMATION SecurityInformation;
        ULONG POINTER_ALIGNMENT Length;
        PVOID SecurityBuffer;
        PVOID MdlAddress; // PMDL
    } QuerySecurity;

    // FLT_PARAMETERS.SetSecurity
    struct
    {
        SECURITY_INFORMATION SecurityInformation;
        PSECURITY_DESCRIPTOR SecurityDescriptor;
    } SetSecurity;

    // FLT_PARAMETERS.WMI
    struct
    {
        ULONG_PTR ProviderId;
        PVOID DataPath;
        ULONG BufferSize;
        PVOID Buffer;
    } SystemControl;

    // FLT_PARAMETERS.QueryQuota
    struct
    {
        ULONG Length;
        PSID StartSid;
        PFILE_GET_QUOTA_INFORMATION SidList;
        ULONG SidListLength;
        PVOID QuotaBuffer;
        PVOID MdlAddress; // PMDL
    } QueryQuota;

    // FLT_PARAMETERS.SetQuota
    struct
    {
        ULONG Length;
        PVOID QuotaBuffer;
        PVOID MdlAddress; // PMDL
    } SetQuota;

    // FLT_PARAMETERS.Pnp
    union
    {
        struct
        {
            PVOID AllocatedResources; // PCM_RESOURCE_LIST
            PVOID AllocatedResourcesTranslated; // PCM_RESOURCE_LIST
        } StartDevice;

        struct
        {
            DEVICE_RELATION_TYPE Type;
        } QueryDeviceRelations;

        struct
        {
            CONST GUID *InterfaceType;
            USHORT Size;
            USHORT Version;
            PVOID Interface; // PINTERFACE
            PVOID InterfaceSpecificData;
        } QueryInterface;

        struct
        {
            PVOID Capabilities; // PDEVICE_CAPABILITIES
        } DeviceCapabilities;

        struct
        {
            PVOID IoResourceRequirementList; // PIO_RESOURCE_REQUIREMENTS_LIST
        } FilterResourceRequirements;

        struct
        {
            ULONG WhichSpace;
            PVOID Buffer;
            ULONG Offset;
            ULONG POINTER_ALIGNMENT Length;
        } ReadWriteConfig;

        struct
        {
            BOOLEAN Lock;
        } SetLock;

        struct
        {
            BUS_QUERY_ID_TYPE IdType;
        } QueryId;

        struct
        {
            DEVICE_TEXT_TYPE DeviceTextType;
            LCID POINTER_ALIGNMENT LocaleId;
        } QueryDeviceText;

        struct
        {
            BOOLEAN InPath;
            BOOLEAN Reserved[3];
            DEVICE_USAGE_NOTIFICATION_TYPE POINTER_ALIGNMENT Type;
        } UsageNotification;
    } Pnp;

     // FLT_PARAMETERS.AcquireForSectionSynchronization
    struct
    {
        FS_FILTER_SECTION_SYNC_TYPE SyncType;
        ULONG PageProtection;
        PVOID OutputInformation; // PFS_FILTER_SECTION_SYNC_OUTPUT
        ULONG Flags;
        ULONG AllocationAttributes;
    } AcquireForSectionSync;

    // FLT_PARAMETERS.AcquireForModifiedPageWriter
    struct
    {
        PLARGE_INTEGER EndingOffset;
        PVOID *ResourceToRelease; // PERESOURCE
    } AcquireForModWrite;

    // FLT_PARAMETERS.ReleaseForModifiedPageWriter
    struct
    {
        PVOID ResourceToRelease; // PERESOURCE
    } ReleaseForModWrite;

    // FLT_PARAMETERS.QueryOpen
    struct
    {
        PIRP Irp;
        PVOID FileInformation;
        PULONG Length;
        FILE_INFORMATION_CLASS FileInformationClass;
    } QueryOpen;

    // FLT_PARAMETERS.FastIoCheckIfPossible
    struct
    {
        LARGE_INTEGER FileOffset;
        ULONG Length;
        ULONG POINTER_ALIGNMENT LockKey;
        BOOLEAN POINTER_ALIGNMENT CheckForReadOperation;
    } FastIoCheckIfPossible;

    // FLT_PARAMETERS.NetworkQueryOpen
    struct
    {
        PIRP Irp;
        PFILE_NETWORK_OPEN_INFORMATION NetworkInformation;
    } NetworkQueryOpen;

    // FLT_PARAMETERS.MdlRead
    struct
    {
        LARGE_INTEGER FileOffset;
        ULONG POINTER_ALIGNMENT Length;
        ULONG POINTER_ALIGNMENT Key;
        PVOID *MdlChain; // PMDL
    } MdlRead;

    // FLT_PARAMETERS.MdlReadComplete
    struct
    {
        PVOID MdlChain; // PMDL
    } MdlReadComplete;

    // FLT_PARAMETERS.PrepareMdlWrite
    struct
    {
        LARGE_INTEGER FileOffset;
        ULONG POINTER_ALIGNMENT Length;
        ULONG POINTER_ALIGNMENT Key;
        PVOID *MdlChain; // PMDL
    } PrepareMdlWrite;

    // FLT_PARAMETERS.MdlWriteComplete
    struct
    {
        LARGE_INTEGER FileOffset;
        PVOID MdlChain; // PMDL
    } MdlWriteComplete;

    // FLT_PARAMETERS.MountVolume
    struct
    {
        ULONG DeviceType;
    } VolumeMount;

    // FLT_PARAMETERS.Others
    struct
    {
        PVOID Argument1;
        PVOID Argument2;
        PVOID Argument3;
        PVOID Argument4;
        PVOID Argument5;
        LARGE_INTEGER Argument6;
    } Others;
} KPHM_FILE_PARAMETERS, *PKPHM_FILE_PARAMETERS;
#pragma warning(pop)

//
// COPY_INFORMATION
//
typedef struct _KPHM_COPY_INFORMATION
{
    PVOID SourceFileObject;
    ULONG64 SourceFileOffset;
} KPHM_COPY_INFORMATION, *PKPHM_COPY_INFORMATION;

typedef struct _KPHM_FILE
{
    CLIENT_ID ClientId;
    ULONG64 ProcessStartKey;
    PVOID ThreadSubProcessTag;

    UCHAR MajorFunction;  // IRP_MJ_*
    UCHAR MinorFunction;  // IRP_MN_*
    UCHAR Irql;           //                        KeGetCurrentIrql
    UCHAR OperationFlags; // SL_*                   IO_STACK_LOCATION.Flags
    ULONG FltFlags;       // FLTFL_CALLBACK_DATA_*  FLT_CALLBACK_DATA.Flags
    ULONG IrpFlags;       // IRP_*                  IRP.Flags
    ULONG Flags;          // FO_*                   FILE_OBJECT.Flags

    union
    {
        ULONG Information;
        struct
        {
            ULONG RequestorMode : 1;       // KernelMode == 0, UserMode == 1
            ULONG IsPagingFile : 1;        // FsRtlIsPagingFile
            ULONG IsSystemPagingFile : 1;  // FsRtlIsSystemPagingFile
            ULONG OriginRemote : 1;        // IoIsFileOriginRemote
            ULONG IgnoringSharing : 1;     // IoIsFileObjectIgnoringSharing
            ULONG InStackFileObject : 1;   // IoGetStackLimits FLT_RELATED_OBJECTS.FileObject
            ULONG LockOperation : 1;       // FILE_OBJECT.LockOperation
            ULONG DeletePending : 1;       // FILE_OBJECT.DeletePending
            ULONG ReadAccess : 1;          // FILE_OBJECT.ReadAccess
            ULONG WriteAccess : 1;         // FILE_OBJECT.WriteAccess
            ULONG DeleteAccess : 1;        // FILE_OBJECT.DeleteAccess
            ULONG SharedRead : 1;          // FILE_OBJECT.SharedRead
            ULONG SharedWrite : 1;         // FILE_OBJECT.SharedWrite
            ULONG SharedDelete : 1;        // FILE_OBJECT.SharedDelete
            ULONG Busy : 1;                // FILE_OBJECT.Busy
            ULONG Spare : 1;
            ULONG TransactionContext : 16; // FLT_RELATED_OBJECTS.TransactionContext
        };
    };

    ULONG Waiters;                         // FILE_OBJECT.Waiters
    LARGE_INTEGER CurrentByteOffset;       // FILE_OBJECT.CurrentByteOffset
    OPLOCK_KEY_CONTEXT OplockKeyContext;   // IoGetOplockKeyContextEx
    KPHM_COPY_INFORMATION CopyInformation; // FltGetCopyInformationFromCallbackData

    union
    {
        ULONG64 Information2;
        struct
        {
            ULONG64 OpenedAsCopySource : 1;      // IoCheckFileObjectOpenedAsCopySource
            ULONG64 OpenedAsCopyDestination : 1; // IoCheckFileObjectOpenedAsCopyDestination
            ULONG64 Spare2 : 62;
        };
    };

    PVOID Volume;             // FLT_RELATED_OBJECTS.Volume
    PVOID FileObject;         // FLT_RELATED_OBJECTS.FileObject
    PVOID Transaction;        // FLT_RELATED_OBJECTS.Transaction
    PVOID DataSectionObject;  // FILE_OBJECT.SectionObjectPointer.DataSectionObject
    PVOID ImageSectionObject; // FILE_OBJECT.SectionObjectPointer.ImageSectionObject

    ULONG64 Sequence;
    KPHM_FILE_PARAMETERS Parameters;

    union
    {
        struct
        {
            union
            {
                struct
                {
                    BOOLEAN MaybeTunneledFileName;
                } Create;

                struct
                {
                    NAMED_PIPE_CREATE_PARAMETERS Parameters;
                } CreateNamedPipe;

                struct
                {
                    BOOLEAN MaybeTunneledFileName;
                    BOOLEAN MaybeTunneledDestinationFileName;
                } SetInformation;

                struct
                {
                    LARGE_INTEGER Length;
                } LockControl;

                struct
                {
                    MAILSLOT_CREATE_PARAMETERS Parameters;
                } CreateMailslot;

                struct
                {
                    LARGE_INTEGER EndingOffset;
                } AcquireForModWrite;

                struct
                {
                    ULONG Length;
                } QueryOpen;
            };
        } Pre;

        struct
        {
            ULONG64 PreSequence;
            LARGE_INTEGER PreTimeStamp;
            IO_STATUS_BLOCK IoStatus;
            union
            {
                struct
                {
                    BOOLEAN TunneledFileName;
                } Create;

                struct
                {
                    BOOLEAN TunneledFileName;
                    BOOLEAN TunneledDestinationFileName;
                } SetInformation;
            };
        } Post;
    };

    //
    // Dynamic
    //
    // id: KphMsgFieldFileName                type: KphMsgTypeUnicodeString
    //
    // id: KphMsgFieldEaBuffer                type: KphMsgTypeSizedBuffer
    //     - KphMsgFilePreCreate
    //
    // id: KphMsgFieldInformationBuffer       type: KphMsgTypeSizedBuffer
    //     - KphMsgFilePreSetInformation
    //     - KphMsgFilePreQueryEa
    //     - KphMsgFilePreSetEa
    //     - KphMsgFilePreSetVolumeInformation
    //     - KphMsgFilePostQueryEa
    //     - KphMsgFilePostQueryVolumeInformation
    //     - KphMsgFilePostDirectoryControl
    //     - KphMsgFilePostQuerySecurity
    //     - KphMsgFilePostQueryOpen
    //     - KphMsgFilePostNetworkQueryOpen
    //
    // id: KphMsgFieldDestinationFileName     type: KphMsgTypeUnicodeString
    //     - KphMsgFilePreDirectoryControl
    //     - KphMsgFilePreSetInformation
    //         - FileInformationClass == FileRename* || FileLink*
    //     - KphMsgFilePostSetInformation
    //         - FileInformationClass == FileRename* || FileLink*
    //
    // id: KphMsgFieldInput                   type: KphMsgTypeSizedBuffer
    //     - KphMsgFilePreFileSystemControl
    //     - KphMsgFilePreDeviceControl
    //     - KphMsgFilePreInternalDeviceControl
    //     - KphMsgFilePostFileSystemControl
    //     - KphMsgFilePostDeviceControl
    //     - KphMsgFilePostInternalDeviceControl
    //
    // id: KphMsgFieldOutput                  type: KphMsgTypeSizedBuffer
    //     - KphMsgFilePreFileSystemControl
    //     - KphMsgFilePreDeviceControl
    //     - KphMsgFilePreInternalDeviceControl
    //     - KphMsgFilePostFileSystemControl
    //     - KphMsgFilePostDeviceControl
    //     - KphMsgFilePostInternalDeviceControl
    //
} KPHM_FILE, *PKPHM_FILE;

typedef struct _KPHM_FILE_REPLY
{
    union
    {
        struct
        {
            struct
            {
                NTSTATUS Status;
            } Create;
        } Pre;

        struct
        {
            struct
            {
                NTSTATUS Status;
            } Create;
        } Post;
    };
} KPHM_FILE_REPLY, *PKPHM_FILE_REPLY;

typedef union _KPHM_REGISTRY_PARAMETERS
{
    struct
    {
        PUNICODE_STRING ValueName;
        ULONG TitleIndex;
        ULONG Type;
        PVOID Data;
        ULONG DataSize;
    } SetValueKey;

    struct
    {
        PUNICODE_STRING ValueName;
    } DeleteValueKey;

    struct
    {
        KEY_SET_INFORMATION_CLASS KeySetInformationClass;
        PVOID KeySetInformation;
        ULONG KeySetInformationLength;
    } SetInformationKey;

    struct
    {
        PUNICODE_STRING NewName;
    } RenameKey;

    struct
    {
        ULONG Index;
        KEY_INFORMATION_CLASS KeyInformationClass;
        PVOID KeyInformation;
        ULONG Length;
        PULONG ResultLength;
    } EnumerateKey;

    struct
    {
        ULONG Index;
        KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass;
        PVOID KeyValueInformation;
        ULONG Length;
        PULONG ResultLength;
    } EnumerateValueKey;

    struct
    {
        KEY_INFORMATION_CLASS KeyInformationClass;
        PVOID KeyInformation;
        ULONG Length;
        PULONG ResultLength;
    } QueryKey;

    struct
    {
        PUNICODE_STRING ValueName;
        KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass;
        PVOID KeyValueInformation;
        ULONG Length;
        PULONG ResultLength;
    } QueryValueKey;

    struct
    {
        PKEY_VALUE_ENTRY ValueEntries;
        ULONG EntryCount;
        PVOID ValueBuffer;
        PULONG BufferLength;
        PULONG RequiredBufferLength;
    } QueryMultipleValueKey;

    struct
    {
        PUNICODE_STRING CompleteName;
        PVOID RootObject;
        PVOID ObjectType;
        ULONG Options;
        PUNICODE_STRING Class;
        PVOID SecurityDescriptor;
        PVOID SecurityQualityOfService;
        ACCESS_MASK DesiredAccess;
        ACCESS_MASK GrantedAccess;
        PULONG Disposition;
        PUNICODE_STRING RemainingName;
        ULONG Wow64Flags;
        ULONG Attributes;
        UCHAR CheckAccessMode;
    } CreateKey;

    struct
    {
        PUNICODE_STRING CompleteName;
        PVOID RootObject;
        PVOID ObjectType;
        ULONG Options;
        PUNICODE_STRING Class;
        PVOID SecurityDescriptor;
        PVOID SecurityQualityOfService;
        ACCESS_MASK DesiredAccess;
        ACCESS_MASK GrantedAccess;
        PULONG Disposition;
        PUNICODE_STRING RemainingName;
        ULONG Wow64Flags;
        ULONG Attributes;
        UCHAR CheckAccessMode;
    } OpenKey;

    struct
    {
        PVOID RootObject;
        PUNICODE_STRING KeyName;
        PUNICODE_STRING SourceFile;
        ULONG Flags;
        PVOID TrustClassObject;
        PVOID UserEvent;
        ACCESS_MASK DesiredAccess;
        PHANDLE RootHandle;
        PVOID FileAccessToken;
    } LoadKey;

    struct
    {
        PVOID UserEvent;
    } UnLoadKey;

    struct
    {
        PSECURITY_INFORMATION SecurityInformation;
        PSECURITY_DESCRIPTOR SecurityDescriptor;
        PULONG Length;
    } QueryKeySecurity;

    struct
    {
        PSECURITY_INFORMATION SecurityInformation;
        PSECURITY_DESCRIPTOR SecurityDescriptor;
    } SetKeySecurity;

    struct
    {
        HANDLE FileHandle;
        ULONG Flags;
    } RestoreKey;

    struct
    {
        HANDLE FileHandle;
        ULONG Format;
    } SaveKey;

    struct
    {
        PUNICODE_STRING OldFileName;
        PUNICODE_STRING NewFileName;
    } ReplaceKey;

    struct
    {
        POBJECT_NAME_INFORMATION ObjectNameInfo;
        ULONG Length;
        PULONG ReturnLength;
    } QueryKeyName;

    struct
    {
        HANDLE FileHandle;
        PVOID HighKeyObject;
        PVOID LowKeyObject;
    } SaveMergedKey;
} KPHM_REGISTRY_PARAMETERS, *PKPHM_REGISTRY_PARAMETERS;

typedef struct _KPHM_REGISTRY
{
    CLIENT_ID ClientId;
    ULONG64 ProcessStartKey;
    PVOID ThreadSubProcessTag;

    union
    {
        USHORT Information;
        struct
        {
            USHORT PostOperation : 1;
            USHORT PreviousMode : 1;       // KernelMode == 0, UserMode == 1
            USHORT Spare : 14;
        };
    };

    PVOID Object;
    ULONG_PTR ObjectId;   // CmCallbackGetKeyObjectIDEx
    PVOID Transaction;    // CmGetBoundTransaction

    ULONG64 Sequence;
    KPHM_REGISTRY_PARAMETERS Parameters;

    union
    {
        struct
        {
            union
            {
                struct
                {
                    ULONG BufferLength;
                } QueryMultipleValueKey;

                struct
                {
                    SECURITY_INFORMATION SecurityInformation;
                    ULONG Length;
                } QueryKeySecurity;

                struct
                {
                    SECURITY_INFORMATION SecurityInformation;
                } SetKeySecurity;

                struct
                {
                    ULONG_PTR LowKeyObjectId;
                    PVOID LowKeyTransaction;
                } SaveMergedKey;
            };
        } Pre;

        struct
        {
            ULONG64 PreSequence;
            LARGE_INTEGER PreTimeStamp;
            NTSTATUS Status;

            union
            {
                struct
                {
                    ULONG ResultLength;
                } EnumerateKey;

                struct
                {
                    ULONG ResultLength;
                } EnumerateValueKey;

                struct
                {
                    ULONG ResultLength;
                } QueryKey;

                struct
                {
                    ULONG ResultLength;
                } QueryValueKey;

                struct
                {
                    ULONG BufferLength;
                    ULONG RequiredBufferLength;
                } QueryMultipleValueKey;

                struct
                {
                    ULONG Disposition;
                } CreateKey;

                struct
                {
                    ULONG Disposition;
                } OpenKey;

                struct
                {
                    HANDLE RootHandle;
                } LoadKey;

                struct
                {
                    ULONG Length;
                } QueryKeySecurity;

                struct
                {
                    ULONG ReturnLength;
                } QueryKeyName;

                struct
                {
                    ULONG_PTR LowKeyObjectId;
                    PVOID LowKeyTransaction;
                } SaveMergedKey;
            };
        } Post;
    };

    //
    // Dynamic
    //
    // id: KphMsgFieldObjectName              type: KphMsgTypeUnicodeString
    //
    // id: KphMsgFieldValueName               type: KphMsgTypeUnicodeString
    //     - KphMsgRegPreSetValueKey
    //     - KphMsgRegPreDeleteValueKey
    //     - KphMsgRegPostSetValueKey
    //     - KphMsgRegPostDeleteValueKey
    //
    // id: KphMsgFieldValueBuffer             type: KphMsgTypeSizedBuffer
    //     - KphMsgRegPreSetValueKey
    //     - KphMsgRegPostQueryValueKey
    //     - KphMsgRegPostQueryMultipleValueKey
    //
    // id: KphMsgFieldInformationBuffer       type: KphMsgTypeSizedBuffer
    //     - KphMsgRegPreSetInformationKey
    //     - KphMsgRegPostEnumerateKey
    //     - KphMsgRegPostEnumerateValueKey
    //     - KphMsgRegPostQueryKey
    //
    // id: KphMsgFieldNewName                 type: KphMsgTypeSizedBuffer
    //     - KphMsgRegPreRenameKey
    //
    // id: KphMsgFieldMultiValueNames         type: KphMsgTypeSizedBuffer
    //     - KphMsgRegPreQueryMultipleValueKey
    //     - KphMsgRegPostQueryMultipleValueKey
    //
    // id: KphMsgFieldMultiValueEntries       type: KphMsgTypeSizedBuffer
    //     - KphMsgRegPostQueryMultipleValueKey
    //
    // id: KphMsgFieldClass                   type: KphMsgTypeUnicodeString
    //     - KphMsgRegPreCreateKey
    //
    // id: KphMsgFieldFileName                type: KphMsgTypeUnicodeString
    //     - KphMsgRegPreLoadKey
    //     - KphMsgRegPreRestoreKey
    //     - KphMsgRegPreSaveKey
    //     - KphMsgRegPreReplaceKey
    //     - KphMsgRegPreSaveMergedKey
    //     - KphMsgRegPostLoadKey
    //     - KphMsgRegPostRestoreKey
    //     - KphMsgRegPostSaveKey
    //     - KphMsgRegPostReplaceKey
    //     - KphMsgRegPostSaveMergedKey
    //
    // id: KphMsgFieldDestinationFileName     type: KphMsgTypeUnicodeString
    //     - KphMsgRegPreRenameKey
    //     - KphMsgRegPostRenameKey
    //
    // id: KphMsgFieldOtherObjectName         type: KphMsgTypeUnicodeString
    //     - KphMsgRegPreSaveMergedKey
    //     - KphMsgRegPostSaveMergedKey
    //
} KPHM_REGISTRY, *PKPHM_REGISTRY;

typedef struct _KPHM_IMAGE_VERIFY
{
    CLIENT_ID ClientId;
    ULONG64 ProcessStartKey;
    PVOID ThreadSubProcessTag;
    ULONG ImageType;               // SE_IMAGE_TYPE
    ULONG Classification;          // BDCB_CLASSIFICATION
    ULONG ImageFlags;
    ULONG ImageHashAlgorithm;
    ULONG ThumbprintHashAlgorithm;

    //
    // Dynamic
    //
    // id: KphMsgFieldFileName                type: KphMsgTypeUnicodeString
    // id: KphMsgFieldHash                    type: KphMsgTypeSizedBuffer
    // id: KphMsgFieldCertificatePublisher    type: KphMsgTypeUnicodeString
    // id: KphMsgFieldCertificateIssuer       type: KphMsgTypeUnicodeString
    // id: KphMsgFieldCertificateThumbprint   type: KphMsgTypeSizedBuffer
    // id: KphMsgFieldRegistryPath            type: KphMsgTypeUnicodeString
    //
} KPHM_IMAGE_VERIFY, *PKPHM_IMAGE_VERIFY;

#pragma warning(pop)

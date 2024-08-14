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

#include <kphmsgdefs.h>

#pragma warning(push)
#pragma warning(disable : 4201)

typedef enum _KPH_MESSAGE_ID
{
    InvalidKphMsg,

    //
    // PH -> KPH
    //

    KphMsgGetInformerSettings,
    KphMsgSetInformerSettings,
    KphMsgOpenProcess,
    KphMsgOpenProcessToken,
    KphMsgOpenProcessJob,
    KphMsgTerminateProcess,
    KphMsgReadVirtualMemory,
    KphMsgOpenThread,
    KphMsgOpenThreadProcess,
    KphMsgCaptureStackBackTraceThread,
    KphMsgEnumerateProcessHandles,
    KphMsgQueryInformationObject,
    KphMsgSetInformationObject,
    KphMsgOpenDriver,
    KphMsgQueryInformationDriver,
    KphMsgQueryInformationProcess,
    KphMsgSetInformationProcess,
    KphMsgSetInformationThread,
    KphMsgSystemControl,
    KphMsgAlpcQueryInformation,
    KphMsgQueryInformationFile,
    KphMsgQueryVolumeInformationFile,
    KphMsgDuplicateObject,
    KphMsgQueryPerformanceCounter,
    KphMsgCreateFile,
    KphMsgQueryInformationThread,
    KphMsgQuerySection,
    KphMsgCompareObjects,
    KphMsgGetMessageTimeouts,
    KphMsgSetMessageTimeouts,
    KphMsgAcquireDriverUnloadProtection,
    KphMsgReleaseDriverUnloadProtection,
    KphMsgGetConnectedClientCount,
    KphMsgActivateDynData,
    KphMsgRequestSessionAccessToken,
    KphMsgAssignProcessSessionToken,
    KphMsgAssignThreadSessionToken,
    KphMsgGetInformerProcessFilter,
    KphMsgSetInformerProcessFilter,
    KphMsgStripProtectedProcessMasks,
    KphMsgQueryVirtualMemory,
    KphMsgQueryHashInformationFile,

    MaxKphMsgClient,
    MaxKphMsgClientAllowed = 0x40000000,

    //
    // KPH <-> PH
    //

    KphMsgProcessCreate,
    KphMsgProcessExit,
    KphMsgThreadCreate,
    KphMsgThreadExecute,
    KphMsgThreadExit,
    KphMsgImageLoad,
    KphMsgDebugPrint,
    KphMsgHandlePreCreateProcess,
    KphMsgHandlePostCreateProcess,
    KphMsgHandlePreDuplicateProcess,
    KphMsgHandlePostDuplicateProcess,
    KphMsgHandlePreCreateThread,
    KphMsgHandlePostCreateThread,
    KphMsgHandlePreDuplicateThread,
    KphMsgHandlePostDuplicateThread,
    KphMsgHandlePreCreateDesktop,
    KphMsgHandlePostCreateDesktop,
    KphMsgHandlePreDuplicateDesktop,
    KphMsgHandlePostDuplicateDesktop,
    KphMsgRequiredStateFailure,
    KphMsgFilePreCreate,
    KphMsgFilePostCreate,
    KphMsgFilePreCreateNamedPipe,
    KphMsgFilePostCreateNamedPipe,
    KphMsgFilePreClose,
    KphMsgFilePostClose,
    KphMsgFilePreRead,
    KphMsgFilePostRead,
    KphMsgFilePreWrite,
    KphMsgFilePostWrite,
    KphMsgFilePreQueryInformation,
    KphMsgFilePostQueryInformation,
    KphMsgFilePreSetInformation,
    KphMsgFilePostSetInformation,
    KphMsgFilePreQueryEa,
    KphMsgFilePostQueryEa,
    KphMsgFilePreSetEa,
    KphMsgFilePostSetEa,
    KphMsgFilePreFlushBuffers,
    KphMsgFilePostFlushBuffers,
    KphMsgFilePreQueryVolumeInformation,
    KphMsgFilePostQueryVolumeInformation,
    KphMsgFilePreSetVolumeInformation,
    KphMsgFilePostSetVolumeInformation,
    KphMsgFilePreDirectoryControl,
    KphMsgFilePostDirectoryControl,
    KphMsgFilePreFileSystemControl,
    KphMsgFilePostFileSystemControl,
    KphMsgFilePreDeviceControl,
    KphMsgFilePostDeviceControl,
    KphMsgFilePreInternalDeviceControl,
    KphMsgFilePostInternalDeviceControl,
    KphMsgFilePreShutdown,
    KphMsgFilePostShutdown,
    KphMsgFilePreLockControl,
    KphMsgFilePostLockControl,
    KphMsgFilePreCleanup,
    KphMsgFilePostCleanup,
    KphMsgFilePreCreateMailslot,
    KphMsgFilePostCreateMailslot,
    KphMsgFilePreQuerySecurity,
    KphMsgFilePostQuerySecurity,
    KphMsgFilePreSetSecurity,
    KphMsgFilePostSetSecurity,
    KphMsgFilePrePower,
    KphMsgFilePostPower,
    KphMsgFilePreSystemControl,
    KphMsgFilePostSystemControl,
    KphMsgFilePreDeviceChange,
    KphMsgFilePostDeviceChange,
    KphMsgFilePreQueryQuota,
    KphMsgFilePostQueryQuota,
    KphMsgFilePreSetQuota,
    KphMsgFilePostSetQuota,
    KphMsgFilePrePnp,
    KphMsgFilePostPnp,
    KphMsgFilePreAcquireForSectionSync,
    KphMsgFilePostAcquireForSectionSync,
    KphMsgFilePreReleaseForSectionSync,
    KphMsgFilePostReleaseForSectionSync,
    KphMsgFilePreAcquireForModWrite,
    KphMsgFilePostAcquireForModWrite,
    KphMsgFilePreReleaseForModWrite,
    KphMsgFilePostReleaseForModWrite,
    KphMsgFilePreAcquireForCcFlush,
    KphMsgFilePostAcquireForCcFlush,
    KphMsgFilePreReleaseForCcFlush,
    KphMsgFilePostReleaseForCcFlush,
    KphMsgFilePreQueryOpen,
    KphMsgFilePostQueryOpen,
    KphMsgFilePreFastIoCheckIfPossible,
    KphMsgFilePostFastIoCheckIfPossible,
    KphMsgFilePreNetworkQueryOpen,
    KphMsgFilePostNetworkQueryOpen,
    KphMsgFilePreMdlRead,
    KphMsgFilePostMdlRead,
    KphMsgFilePreMdlReadComplete,
    KphMsgFilePostMdlReadComplete,
    KphMsgFilePrePrepareMdlWrite,
    KphMsgFilePostPrepareMdlWrite,
    KphMsgFilePreMdlWriteComplete,
    KphMsgFilePostMdlWriteComplete,
    KphMsgFilePreVolumeMount,
    KphMsgFilePostVolumeMount,
    KphMsgFilePreVolumeDismount,
    KphMsgFilePostVolumeDismount,
    KphMsgRegPreDeleteKey,
    KphMsgRegPostDeleteKey,
    KphMsgRegPreSetValueKey,
    KphMsgRegPostSetValueKey,
    KphMsgRegPreDeleteValueKey,
    KphMsgRegPostDeleteValueKey,
    KphMsgRegPreSetInformationKey,
    KphMsgRegPostSetInformationKey,
    KphMsgRegPreRenameKey,
    KphMsgRegPostRenameKey,
    KphMsgRegPreEnumerateKey,
    KphMsgRegPostEnumerateKey,
    KphMsgRegPreEnumerateValueKey,
    KphMsgRegPostEnumerateValueKey,
    KphMsgRegPreQueryKey,
    KphMsgRegPostQueryKey,
    KphMsgRegPreQueryValueKey,
    KphMsgRegPostQueryValueKey,
    KphMsgRegPreQueryMultipleValueKey,
    KphMsgRegPostQueryMultipleValueKey,
    KphMsgRegPreKeyHandleClose,
    KphMsgRegPostKeyHandleClose,
    KphMsgRegPreCreateKey,
    KphMsgRegPostCreateKey,
    KphMsgRegPreOpenKey,
    KphMsgRegPostOpenKey,
    KphMsgRegPreFlushKey,
    KphMsgRegPostFlushKey,
    KphMsgRegPreLoadKey,
    KphMsgRegPostLoadKey,
    KphMsgRegPreUnLoadKey,
    KphMsgRegPostUnLoadKey,
    KphMsgRegPreQueryKeySecurity,
    KphMsgRegPostQueryKeySecurity,
    KphMsgRegPreSetKeySecurity,
    KphMsgRegPostSetKeySecurity,
    KphMsgRegPreRestoreKey,
    KphMsgRegPostRestoreKey,
    KphMsgRegPreSaveKey,
    KphMsgRegPostSaveKey,
    KphMsgRegPreReplaceKey,
    KphMsgRegPostReplaceKey,
    KphMsgRegPreQueryKeyName,
    KphMsgRegPostQueryKeyName,
    KphMsgRegPreSaveMergedKey,
    KphMsgRegPostSaveMergedKey,
    KphMsgImageVerify,

    MaxKphMsg,

    KphMsgUnhandled = 0xffffffff,
} KPH_MESSAGE_ID, *PKPH_MESSAGE_ID;

C_ASSERT(sizeof(KPH_MESSAGE_ID) == 4);
C_ASSERT(MaxKphMsg > 0);

typedef enum _KPH_MESSAGE_FIELD_ID
{
    InvalidKphMsgField,

    KphMsgFieldFileName,
    KphMsgFieldCommandLine,
    KphMsgFieldOutput,
    KphMsgFieldObjectName,
    KphMsgFieldStackTrace,
    KphMsgFieldDestinationFileName,
    KphMsgFieldEaBuffer,
    KphMsgFieldInformationBuffer,
    KphMsgFieldInput,
    KphMsgFieldValueName,
    KphMsgFieldValueBuffer,
    KphMsgFieldMultipleValueNames,
    KphMsgFieldMultipleValueEntries,
    KphMsgFieldNewName,
    KphMsgFieldClass,
    KphMsgFieldOtherObjectName,
    KphMsgFieldHash,
    KphMsgFieldRegistryPath,
    KphMsgFieldCertificatePublisher,
    KphMsgFieldCertificateIssuer,
    KphMsgFieldCertificateThumbprint,

    MaxKphMsgField
} KPH_MESSAGE_FIELD_ID, *PKPH_MESSAGE_FIELD_ID;

C_ASSERT(sizeof(KPH_MESSAGE_FIELD_ID) == 4);
C_ASSERT(MaxKphMsgField > 0);

typedef enum _KPH_MESSAGE_TYPE_ID
{
    InvalidKphMsgType,

    KphMsgTypeUnicodeString,
    KphMsgTypeAnsiString,
    KphMsgTypeStackTrace,
    KphMsgTypeSizedBuffer,

    MaxKphMsgType
} KPH_MESSAGE_TYPE_ID, *PKPH_MESSAGE_TYPE_ID;

C_ASSERT(sizeof(KPH_MESSAGE_TYPE_ID) == 4);
C_ASSERT(MaxKphMsgType > 0);

typedef struct _KPH_MESSAGE_DYNAMIC_TABLE_ENTRY
{
    KPH_MESSAGE_FIELD_ID FieldId;
    KPH_MESSAGE_TYPE_ID TypeId;
    USHORT Offset;
    USHORT Length;
} KPH_MESSAGE_DYNAMIC_TABLE_ENTRY, *PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY;

typedef const KPH_MESSAGE_DYNAMIC_TABLE_ENTRY* PCKPH_MESSAGE_DYNAMIC_TABLE_ENTRY;

typedef struct _KPH_MESSAGE
{
    struct
    {
        USHORT Version;
        USHORT Size;
        KPH_MESSAGE_ID MessageId;
        LARGE_INTEGER TimeStamp;
    } Header;

    union
    {
        //
        // Message from user
        //
        union
        {
            KPHM_GET_INFORMER_SETTINGS GetInformerSettings;
            KPHM_SET_INFORMER_SETTINGS SetInformerSettings;
            KPHM_OPEN_PROCESS OpenProcess;
            KPHM_OPEN_PROCESS_TOKEN OpenProcessToken;
            KPHM_OPEN_PROCESS_JOB OpenProcessJob;
            KPHM_TERMINATE_PROCESS TerminateProcess;
            KPHM_READ_VIRTUAL_MEMORY ReadVirtualMemory;
            KPHM_OPEN_THREAD OpenThread;
            KPHM_OPEN_THREAD_PROCESS OpenThreadProcess;
            KPHM_CAPTURE_STACK_BACKTRACE_THREAD CaptureStackBackTraceThread;
            KPHM_ENUMERATE_PROCESS_HANDLES EnumerateProcessHandles;
            KPHM_QUERY_INFORMATION_OBJECT QueryInformationObject;
            KPHM_SET_INFORMATION_OBJECT SetInformationObject;
            KPHM_OPEN_DRIVER OpenDriver;
            KPHM_QUERY_INFORMATION_DRIVER QueryInformationDriver;
            KPHM_QUERY_INFORMATION_PROCESS QueryInformationProcess;
            KPHM_SET_INFORMATION_PROCESS SetInformationProcess;
            KPHM_SET_INFORMATION_THREAD SetInformationThread;
            KPHM_SYSTEM_CONTROL SystemControl;
            KPHM_ALPC_QUERY_INFORMATION AlpcQueryInformation;
            KPHM_QUERY_INFORMATION_FILE QueryInformationFile;
            KPHM_QUERY_VOLUME_INFORMATION_FILE QueryVolumeInformationFile;
            KPHM_DUPLICATE_OBJECT DuplicateObject;
            KPHM_QUERY_PERFORMANCE_COUNTER QueryPerformanceCounter;
            KPHM_CREATE_FILE CreateFile;
            KPHM_QUERY_INFORMATION_THREAD QueryInformationThread;
            KPHM_QUERY_SECTION QuerySection;
            KPHM_COMPARE_OBJECTS CompareObjects;
            KPHM_GET_MESSAGE_TIMEOUTS GetMessageTimeouts;
            KPHM_SET_MESSAGE_TIMEOUTS SetMessageTimeouts;
            KPHM_ACQUIRE_DRIVER_UNLOAD_PROTECTION AcquireDriverUnloadProtection;
            KPHM_RELEASE_DRIVER_UNLOAD_PROTECTION ReleaseDriverUnloadProtection;
            KPHM_GET_CONNECTED_CLIENT_COUNT GetConnectedClientCount;
            KPHM_ACTIVATE_DYNDATA ActivateDynData;
            KPHM_REQUEST_SESSION_ACCESS_TOKEN RequestSessionAccessToken;
            KPHM_ASSIGN_PROCESS_SESSION_TOKEN AssignProcessSessionToken;
            KPHM_ASSIGN_THREAD_SESSION_TOKEN AssignThreadSessionToken;
            KPHM_GET_INFORMER_PROCESS_FILTER GetInformerProcessFilter;
            KPHM_SET_INFORMER_PROCESS_FILTER SetInformerProcessFilter;
            KPHM_STRIP_PROTECTED_PROCESS_MASKS StripProtectedProcessMasks;
            KPHM_QUERY_VIRTUAL_MEMORY QueryVirtualMemory;
            KPHM_QUERY_HASH_INFORMATION_FILE QueryHashInformationFile;
        } User;

        //
        // Message from kernel
        //
        union
        {
            KPHM_PROCESS_CREATE ProcessCreate;
            KPHM_PROCESS_EXIT ProcessExit;
            KPHM_THREAD_CREATE ThreadCreate;
            KPHM_THREAD_EXECUTE ThreadExecute;
            KPHM_THREAD_EXIT ThreadExit;
            KPHM_IMAGE_LOAD ImageLoad;
            KPHM_DEBUG_PRINT DebugPrint;
            KPHM_HANDLE Handle;
            KPHM_REQUIRED_STATE_FAILURE RequiredStateFailure;
            KPHM_FILE File;
            KPHM_REGISTRY Reg;
            KPHM_IMAGE_VERIFY ImageVerify;
        } Kernel;

        //
        // Reply to kernel
        //
        union
        {
            KPHM_PROCESS_CREATE_REPLY ProcessCreate;
            KPHM_FILE_REPLY File;
        } Reply;
    };

    //
    // Dynamic data accessed through KphMsgDyn APIs
    //
    struct
    {
        UCHAR Count;
        KPH_MESSAGE_DYNAMIC_TABLE_ENTRY Entries[8];
        CHAR Buffer[0x1000 - 356];
    } _Dyn;
} KPH_MESSAGE, *PKPH_MESSAGE;
typedef const KPH_MESSAGE* PCKPH_MESSAGE;

#define KPH_MESSAGE_MIN_SIZE RTL_SIZEOF_THROUGH_FIELD(KPH_MESSAGE, _Dyn.Entries)

//
// ABI breaking asserts. KPH_MESSAGE_VERSION must be updated.
// const int value = sizeof(KPH_MESSAGE);
// const int value = FIELD_OFFSET(KPH_MESSAGE, _Dyn);
// const int value = FIELD_OFFSET(KPH_MESSAGE, _Dyn.Buffer);
// const int value = KPH_MESSAGE_MIN_SIZE;
//
C_ASSERT(sizeof(KPH_MESSAGE) <= 0xffff);
#ifdef _WIN64
C_ASSERT(sizeof(KPH_MESSAGE) == 0x1000);
C_ASSERT(FIELD_OFFSET(KPH_MESSAGE, _Dyn) == 256);
C_ASSERT(FIELD_OFFSET(KPH_MESSAGE, _Dyn.Buffer) == 356);
C_ASSERT(KPH_MESSAGE_MIN_SIZE == 356);
C_ASSERT(KPH_MESSAGE_MIN_SIZE == FIELD_OFFSET(KPH_MESSAGE, _Dyn.Buffer));
#else
// not supported
#endif

#pragma warning(pop)

EXTERN_C_START

VOID KphMsgInit(
    _Out_writes_bytes_(KPH_MESSAGE_MIN_SIZE) PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_ID MessageId
    );

_Must_inspect_result_
NTSTATUS KphMsgValidate(
    _In_ PCKPH_MESSAGE Message
    );

EXTERN_C_END

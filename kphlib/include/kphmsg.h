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
#include <kphlibbase.h>
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
    KphMsgReadVirtualMemoryUnsafe,
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

    MaxKphMsgClient,
    MaxKphMsgClientAllowed = 0x40000000,

    //
    // KPH -> PH
    //

    KphMsgProcessCreate,
    KphMsgProcessExit,
    KphMsgThreadCreate,
    KphMsgThreadExecute,
    KphMsgThreadExit,
    KphMsgImageLoad,
    KphMsgDebugPrint,
    KphMsgProcessHandlePreCreate,
    KphMsgProcessHandlePostCreate,
    KphMsgProcessHandlePreDuplicate,
    KphMsgProcessHandlePostDuplicate,
    KphMsgThreadHandlePreCreate,
    KphMsgThreadHandlePostCreate,
    KphMsgThreadHandlePreDuplicate,
    KphMsgThreadHandlePostDuplicate,
    KphMsgDesktopHandlePreCreate,
    KphMsgDesktopHandlePostCreate,
    KphMsgDesktopHandlePreDuplicate,
    KphMsgDesktopHandlePostDuplicate,

    MaxKphMsg

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

    MaxKphMsgType

} KPH_MESSAGE_TYPE_ID, *PKPH_MESSAGE_TYPE_ID;

C_ASSERT(sizeof(KPH_MESSAGE_TYPE_ID) == 4);
C_ASSERT(MaxKphMsgType > 0);

typedef struct _KPH_MESSAGE_DYNAMIC_TABLE_ENTRY
{
    KPH_MESSAGE_FIELD_ID FieldId;
    KPH_MESSAGE_TYPE_ID TypeId;
    ULONG Offset;
    ULONG Size;

} KPH_MESSAGE_DYNAMIC_TABLE_ENTRY, *PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY;

typedef const KPH_MESSAGE_DYNAMIC_TABLE_ENTRY* PCKPH_MESSAGE_DYNAMIC_TABLE_ENTRY;

typedef struct _KPH_MESSAGE
{
    struct
    {
        USHORT Version;
        KPH_MESSAGE_ID MessageId;
        ULONG Size;
        LARGE_INTEGER TimeStamp;

    } Header;

    union
    {
        //
        // Message form user
        //
        union
        {
            KPHM_GET_INFORMER_SETTINGS GetInformerSettings;
            KPHM_SET_INFORMER_SETTINGS SetInformerSettings;
            KPHM_OPEN_PROCESS OpenProcess;
            KPHM_OPEN_PROCESS_TOKEN OpenProcessToken;
            KPHM_OPEN_PROCESS_JOB OpenProcessJob;
            KPHM_TERMINATE_PROCESS TerminateProcess;
            KPHM_READ_VIRTUAL_MEMORY_UNSAFE ReadVirtualMemoryUnsafe;
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
            KPHM_PROCESS_HANDLE_PRE_CREATE ProcessHandlePreCreate;
            KPHM_PROCESS_HANDLE_POST_CREATE ProcessHandlePostCreate;
            KPHM_PROCESS_HANDLE_PRE_DUPLICATE ProcessHandlePreDuplicate;
            KPHM_PROCESS_HANDLE_POST_DUPLICATE ProcessHandlePostDuplicate;
            KPHM_THREAD_HANDLE_PRE_CREATE ThreadHandlePreCreate;
            KPHM_THREAD_HANDLE_POST_CREATE ThreadHandlePostCreate;
            KPHM_THREAD_HANDLE_PRE_DUPLICATE ThreadHandlePreDuplicate;
            KPHM_THREAD_HANDLE_POST_DUPLICATE ThreadHandlePostDuplicate;
            KPHM_DESKTOP_HANDLE_PRE_CREATE DesktopHandlePreCreate;
            KPHM_DESKTOP_HANDLE_POST_CREATE DesktopHandlePostCreate;
            KPHM_DESKTOP_HANDLE_PRE_DUPLICATE DesktopHandlePreDuplicate;
            KPHM_DESKTOP_HANDLE_POST_DUPLICATE DesktopHandlePostDuplicate;

        } Kernel;

        //
        // Reply to kernel
        //
        union
        {
            KPHM_PROCESS_CREATE_REPLY ProcessCreate;

        } Reply;
    };

    //
    // Dynamic data accessed through KphMsgDyn APIs
    //
    struct
    {
        USHORT Count;
        KPH_MESSAGE_DYNAMIC_TABLE_ENTRY Entries[8];
        CHAR Buffer[3 * 1024];
    } _Dyn;

} KPH_MESSAGE, *PKPH_MESSAGE;

typedef const KPH_MESSAGE* PCKPH_MESSAGE;

#define KPH_MESSAGE_MIN_SIZE RTL_SIZEOF_THROUGH_FIELD(KPH_MESSAGE, _Dyn.Entries)

#pragma warning(pop)

#ifdef __cplusplus
extern "C" {
#endif

VOID KphMsgInit(
    _Out_writes_bytes_(KPH_MESSAGE_MIN_SIZE) PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_ID MessageId
    );

_Must_inspect_result_
NTSTATUS KphMsgValidate(
    _In_ PCKPH_MESSAGE Message
    );

#ifdef __cplusplus
}
#endif

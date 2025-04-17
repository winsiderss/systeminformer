/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#ifndef ETWMON_H
#define ETWMON_H

#include <evntcons.h>

#define KERNEL_FILE_KEYWORD_FILENAME 0x10
#define KERNEL_FILE_KEYWORD_FILEIO 0x20
#define KERNEL_FILE_KEYWORD_OP_END 0x40
#define KERNEL_FILE_KEYWORD_CREATE 0x80
#define KERNEL_FILE_KEYWORD_READ 0x100
#define KERNEL_FILE_KEYWORD_WRITE 0x200
#define KERNEL_FILE_KEYWORD_DELETE_PATH 0x400
#define KERNEL_FILE_KEYWORD_RENAME_SETLINK_PATH 0x800
#define KERNEL_FILE_KEYWORD_CREATE_NEW_FILE 0x1000

// etwmon

VOID EtEtwMonitorInitialization(
    VOID
    );

VOID EtEtwMonitorUninitialization(
    VOID
    );

VOID EtStartEtwSession(
    VOID
    );

VOID EtStopEtwSession(
    VOID
    );

VOID EtStartEtwRundown(
    VOID
    );

// etwstat

#define EVENT_TRACE_TYPE_TCPIP_SEND_IPV6 0x1A
#define EVENT_TRACE_TYPE_TCPIP_RECEIVE_IPV6 0x1B

#define EVENT_TRACE_TYPE_FILENAME 0x0
#define EVENT_TRACE_TYPE_FILENAME_CREATE 0x20
#define EVENT_TRACE_TYPE_FILENAME_SAME 0x21
#define EVENT_TRACE_TYPE_FILENAME_NULL 0x22
#define EVENT_TRACE_TYPE_FILENAME_DELETE 0x23
#define EVENT_TRACE_TYPE_FILENAME_RUNDOWN 0x24

typedef enum _ET_ETW_EVENT_TYPE
{
    EtEtwDiskReadType = 1,
    EtEtwDiskWriteType,
    EtEtwFileNameType,
    EtEtwFileCreateType,
    EtEtwFileDeleteType,
    EtEtwFileRundownType,
    EtEtwNetworkReceiveType,
    EtEtwNetworkSendType
} ET_ETW_EVENT_TYPE;

typedef struct _ET_ETW_DISK_EVENT
{
    ULONG Type;
    ULONG IrpFlags;
    ULONG TransferSize;
    CLIENT_ID ClientId;
    PVOID FileObject;
    ULONGLONG HighResResponseTime;
} ET_ETW_DISK_EVENT, *PET_ETW_DISK_EVENT;

typedef struct _ET_ETW_FILE_EVENT
{
    ULONG Type;
    PVOID FileObject;
    PH_STRINGREF FileName;
} ET_ETW_FILE_EVENT, *PET_ETW_FILE_EVENT;

typedef struct _ET_ETW_NETWORK_EVENT
{
    ULONG Type;
    ULONG ProtocolType;
    ULONG TransferSize;
    CLIENT_ID ClientId;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;
} ET_ETW_NETWORK_EVENT, *PET_ETW_NETWORK_EVENT;

typedef struct _ET_ETW_STACKWALK_EVENT
{
    ULONGLONG EventTimeStamp;
    ULONG StackProcess;
    ULONG StackThread;
    ULONG_PTR Stack[192];
} ET_ETW_STACKWALK_EVENT, *PET_ETW_STACKWALK_EVENT;
// etwstat

VOID EtProcessDiskEvent(
    _In_ PET_ETW_DISK_EVENT Event
    );

VOID EtProcessNetworkEvent(
    _In_ PET_ETW_NETWORK_EVENT Event
    );

HANDLE EtThreadIdToProcessId(
    _In_ HANDLE ThreadId
    );

// etwdisk

VOID EtDiskProcessDiskEvent(
    _In_ PET_ETW_DISK_EVENT Event
    );

VOID EtDiskProcessFileEvent(
    _In_ PET_ETW_FILE_EVENT Event
    );

#endif

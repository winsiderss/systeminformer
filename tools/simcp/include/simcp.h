/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#ifndef _SIMCP_H
#define _SIMCP_H

#define SIMCP_MAGIC ('CMIS')
#define SIMCP_VERSION 1
#define SIMCP_MAX_PAYLOAD_LENGTH (16 * 1024 * 1024)
#define SIMCP_PIPE_NAME_PREFIX L"SystemInformer.AgentTools."
#define SIMCP_PIPE_PROTECTED_PREFIX L"ProtectedPrefix\\Administrators\\"
#define SIMCP_BROKER_FILE_NAME L"simcp.exe"
#define SIMCP_SERVER_FILE_NAME L"SystemInformer.exe"
typedef enum _SIMCP_MESSAGE_TYPE
{
    SimcpHello = 1,
    SimcpHelloAck = 2,
    SimcpMcp = 3,
    SimcpClose = 4,
} SIMCP_MESSAGE_TYPE;

#include <pshpack4.h>

typedef struct _SIMCP_HEADER
{
    ULONG Magic;
    USHORT Version;
    USHORT Type;
    ULONG PayloadLength;
    ULONG Reserved;
} SIMCP_HEADER, *PSIMCP_HEADER;

typedef struct _SIMCP_HELLO
{
    ULONG BrokerVersion;
    ULONG BrokerProcessId;
    ULONG LauncherProcessId;
    ULONG Reserved;
    LARGE_INTEGER LauncherStartTime;
} SIMCP_HELLO, *PSIMCP_HELLO;

typedef enum _SIMCP_HELLO_STATUS
{
    SimcpHelloAccepted = 0,
    SimcpHelloRejectedVersion = 1,
    SimcpHelloRejectedUser = 2,
    SimcpHelloRejectedIntegrity = 3,
    SimcpHelloRejectedAppContainer = 4,
    SimcpHelloRejectedImage = 5,
    SimcpHelloRejectedSignature = 6,
    SimcpHelloRejectedProcessId = 7,
    SimcpHelloRejectedInternal = 8,
    SimcpHelloRejectedByUser = 9,
} SIMCP_HELLO_STATUS;

typedef struct _SIMCP_HELLO_ACK
{
    ULONG Status;
    ULONG ConnectionId;
    ULONG ServerVersion[4];
    ULONG Reserved;
} SIMCP_HELLO_ACK, *PSIMCP_HELLO_ACK;

typedef enum _SIMCP_CLOSE_REASON
{
    SimcpCloseUserDisconnected = 1,
    SimcpCloseServerDisabled = 2,
    SimcpCloseServerShutdown = 3,
    SimcpCloseProtocolViolation = 4,
    SimcpCloseRejected = 5,
} SIMCP_CLOSE_REASON;

typedef struct _SIMCP_CLOSE
{
    ULONG Reason;
    ULONG Detail;
} SIMCP_CLOSE, *PSIMCP_CLOSE;

#include <poppack.h>

#endif

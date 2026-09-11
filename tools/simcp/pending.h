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

#ifndef _SIMCP_PENDING_H
#define _SIMCP_PENDING_H

// A host that never reads its replies must not grow the broker without bound.
#define SIMCP_MAX_PENDING_REQUESTS 1024

typedef struct _SIMCP_PENDING
{
    PPH_HASHTABLE Table;
    PH_QUEUED_LOCK Lock;    // the stdin thread adds, the pipe thread removes
} SIMCP_PENDING, *PSIMCP_PENDING;

VOID SimcpInitializePending(
    _Out_ PSIMCP_PENDING Pending
    );

VOID SimcpDeletePending(
    _Inout_ PSIMCP_PENDING Pending
    );

_Success_(return)
BOOLEAN SimcpAddPending(
    _Inout_ PSIMCP_PENDING Pending,
    _In_ PPH_BYTES Id
    );

BOOLEAN SimcpRemovePending(
    _Inout_ PSIMCP_PENDING Pending,
    _In_ PPH_BYTES Id
    );

VOID SimcpClearPending(
    _Inout_ PSIMCP_PENDING Pending
    );

PPH_LIST SimcpTakePending(
    _Inout_ PSIMCP_PENDING Pending
    );

ULONG SimcpPendingCount(
    _In_ PSIMCP_PENDING Pending
    );

#endif

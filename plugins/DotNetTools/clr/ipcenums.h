/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2016
 *
 */

// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the current folder for more information.
//-----------------------------------------------------------------------------
// IPCEnums.h
//
// Define various enums used by IPCMan.
//-----------------------------------------------------------------------------
//
// dmex: This header has been highly modified.
// Original: https://github.com/dotnet/coreclr/blob/master/src/ipcman/ipcenums.h

#ifndef _IPC_ENUMS_H_
#define _IPC_ENUMS_H_

//-----------------------------------------------------------------------------
// Each IPC client for an IPC block has one entry.
//-----------------------------------------------------------------------------
typedef enum _EIPCClient
{
    eIPC_PerfCounters = 0,

    // MAX used for arrays, insert above this.
    eIPC_MAX
} EIPCClient;

//-----------------------------------------------------------------------------
// Each IPC client for a LegacyPrivate block (debugging, perf counters, etc) has one entry.
//-----------------------------------------------------------------------------
typedef enum _ELegacyPrivateIPCClient
{
    eLegacyPrivateIPC_PerfCounters = 0,
    eLegacyPrivateIPC_Obsolete_Debugger = 1,
    eLegacyPrivateIPC_AppDomain = 2,
    eLegacyPrivateIPC_Obsolete_Service = 3,
    eLegacyPrivateIPC_Obsolete_ClassDump = 4,
    eLegacyPrivateIPC_Obsolete_MiniDump = 5,
    eLegacyPrivateIPC_InstancePath = 6,

    // MAX used for arrays, insert above this.
    eLegacyPrivateIPC_MAX
} ELegacyPrivateIPCClient;

//-----------------------------------------------------------------------------
// Each IPC client for a LegacyPublic block has one entry.
//-----------------------------------------------------------------------------
typedef enum _ELegacyPublicIPCClient
{
    eLegacyPublicIPC_PerfCounters = 0,

    // MAX used for arrays, insert above this.
    eLegacyPublicIPC_MAX
} ELegacyPublicIPCClient;

#endif _IPC_ENUMS_H_
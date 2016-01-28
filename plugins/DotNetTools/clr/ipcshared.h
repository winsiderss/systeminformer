/*
 * Process Hacker .NET Tools -
 *   .NET Process IPC definitions
 *
 * Copyright (C) 2015-2016 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

// Licensed to the .NET Foundation under one or more agreements. 
// The .NET Foundation licenses this file to you under the MIT license. 
// See the LICENSE file in the current folder for more information. 
//----------------------------------------------------------------------------- 
// IPCShared.h 
// 
// Shared LegacyPrivate utility functions for IPC operations.
//----------------------------------------------------------------------------- 
//
// dmex: This header has been highly modified.
// Original: https://github.com/dotnet/coreclr/blob/master/src/ipcman/ipcshared.h

#ifndef _IPC_SHARED_H_
#define _IPC_SHARED_H_

// This is the name of the file backed session's name on the LS (debuggee)
// Name of the LegacyPrivate (per-process) block. %lu resolved to a PID
#define CorLegacyPrivateIPCBlock            L"Cor_Private_IPCBlock_%lu"
#define CorLegacyPrivateIPCBlockTempV4      L"Cor_Private_IPCBlock_v4_%lu"
#define CorLegacyPublicIPCBlock             L"Cor_Public_IPCBlock_%lu"
#define CorSxSPublicIPCBlock                L"Cor_SxSPublic_IPCBlock_%lu"
#define CorSxSBoundaryDescriptor            L"Cor_CLR_IPCBlock_%lu"
#define CorSxSWriterPrivateNamespacePrefix  L"Cor_CLR_WRITER"
#define CorSxSReaderPrivateNamespacePrefix  L"Cor_CLR_READER"
#define CorSxSVistaPublicIPCBlock           L"Cor_SxSPublic_IPCBlock"

#define CorLegacyPrivateIPCBlock_RS         L"CLR_PRIVATE_RS_IPCBlock_%lu"
#define CorLegacyPrivateIPCBlock_RSTempV4   L"CLR_PRIVATE_RS_IPCBlock_v4_%lu"
#define CorLegacyPublicIPCBlock_RS          L"CLR_PUBLIC_IPCBlock_%lu"
#define CorSxSPublicIPCBlock_RS             L"CLR_SXSPUBLIC_IPCBlock_%lu"

#define CorSxSPublicInstanceName            L"%s_p%lu_r%lu"
#define CorSxSPublicInstanceNameWhidbey     L"%s_p%lu"

#endif _IPC_SHARED_H_
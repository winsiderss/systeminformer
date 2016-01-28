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
//
// dmex: This header has been highly modified.
// Original: https://github.com/dotnet/coreclr/blob/master/src/debug/inc/dbgappdomain.h

#ifndef _DBG_APPDOMAIN_H_
#define _DBG_APPDOMAIN_H_

#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct _AppDomainInfo
{
    ULONG Id;             // unique identifier
    INT NameLengthInBytes;
    PWSTR AppDomainName;
    PVOID AppDomains;
} AppDomainInfo;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct _AppDomainInfo_Wow64
{
    ULONG Id;             // unique identifier
    INT NameLengthInBytes;
    ULONG AppDomainName;
    ULONG AppDomains;
} AppDomainInfo_Wow64;
#include <poppack.h>

// AppDomain publishing server support:
// Information about all appdomains in the process will be maintained
// in the shared memory block for use by the debugger, etc.
#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct _AppDomainEnumerationIPCBlock
{
    HANDLE Mutex;                 // lock for serialization while manipulating AppDomain list.
 
    INT TotalSlots;                  // Number of slots in AppDomainListElement array
    INT NumOfUsedSlots;
    INT LastFreedSlot;
    INT SizeInBytes;                 // Size of AppDomainInfo in bytes

    INT ProcessNameLengthInBytes;    // We can use psapi!GetModuleFileNameEx to get the module name.
    PVOID ProcessName;               // This provides an alternative.

    PVOID ListOfAppDomains;
    BOOL LockInvalid;
} AppDomainEnumerationIPCBlock;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct _AppDomainEnumerationIPCBlock_Wow64
{
    ULONG Mutex;                    // lock for serialization while manipulating AppDomain list.

    INT TotalSlots;                 // Number of slots in AppDomainListElement array
    INT NumOfUsedSlots;
    INT LastFreedSlot;
    INT SizeInBytes;                // Size of AppDomainInfo in bytes

    INT ProcessNameLengthInBytes;   // We can use psapi!GetModuleFileNameEx to get the module name.
    ULONG ProcessName;              // This provides an alternative.

    ULONG ListOfAppDomains;
    BOOL LockInvalid;
} AppDomainEnumerationIPCBlock_Wow64;
#include <poppack.h>


// Enforce the AppDomain IPC block binary layout.
C_ASSERT(FIELD_OFFSET(AppDomainInfo_Wow64, Id) == 0x0);
C_ASSERT(FIELD_OFFSET(AppDomainInfo_Wow64, NameLengthInBytes) == 0x4);
C_ASSERT(FIELD_OFFSET(AppDomainInfo_Wow64, AppDomainName) == 0x8);
C_ASSERT(FIELD_OFFSET(AppDomainInfo_Wow64, AppDomains) == 0xC);

#if defined(_WIN64)
C_ASSERT(FIELD_OFFSET(AppDomainInfo, Id) == 0x0);
C_ASSERT(FIELD_OFFSET(AppDomainInfo, NameLengthInBytes) == 0x4);
C_ASSERT(FIELD_OFFSET(AppDomainInfo, AppDomainName) == 0x8);
C_ASSERT(FIELD_OFFSET(AppDomainInfo, AppDomains) == 0x10);
#endif

// Enforce the AppDomain IPC block binary layout.
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock_Wow64, Mutex) == 0x0);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock_Wow64, TotalSlots) == 0x4);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock_Wow64, NumOfUsedSlots) == 0x8);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock_Wow64, LastFreedSlot) == 0xC);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock_Wow64, SizeInBytes) == 0x10);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock_Wow64, ProcessNameLengthInBytes) == 0x14);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock_Wow64, ProcessName) == 0x18);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock_Wow64, ListOfAppDomains) == 0x1C);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock_Wow64, LockInvalid) == 0x20);

#if defined(_WIN64)
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock, Mutex) == 0x0);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock, TotalSlots) == 0x8);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock, NumOfUsedSlots) == 0xC);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock, LastFreedSlot) == 0x10);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock, SizeInBytes) == 0x14);
C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock, ProcessNameLengthInBytes) == 0x18);
// TODO: Why does Visual Studio have issues with these entries...
//C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock, ProcessName) == 0x20);
//C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock, ListOfAppDomains) == 0x28);
//C_ASSERT(FIELD_OFFSET(AppDomainEnumerationIPCBlock, LockInvalid) == 0x30);
#endif

#endif _DBG_APPDOMAIN_H_
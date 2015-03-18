/*
 * Process Hacker Extra Plugins -
 *   DNS Cache Plugin
 *
 * Copyright (C) 2014 dmex
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

#ifndef _DNS_H_
#define _DNS_H_

#define SETTING_PREFIX L"dmex.DnsCachePlugin"
#define SETTING_NAME_WINDOW_POSITION (SETTING_PREFIX L".WindowPosition")
#define SETTING_NAME_WINDOW_SIZE (SETTING_PREFIX L".WindowSize")
#define SETTING_NAME_COLUMNS (SETTING_PREFIX L".WindowColumns")

#define DNSCACHE_MENUITEM   1000
#define INET_ADDRSTRLEN     22
#define INET6_ADDRSTRLEN    65

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include <colmgr.h>
#include "resource.h"

#include <windns.h>
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")

typedef struct _DNS_CACHE_ENTRY
{
    struct _DNS_CACHE_ENTRY* Next;  // Pointer to next entry
    PCWSTR Name;                    // DNS Record Name
    USHORT Type;                    // DNS Record Type
    USHORT DataLength;              // Not referenced
    ULONG Flags;                    // DNS Record Flags
} DNS_CACHE_ENTRY, *PDNS_CACHE_ENTRY;

typedef DNS_STATUS (WINAPI* _DnsGetCacheDataTable)(
    _Inout_ PDNS_CACHE_ENTRY* DnsCacheEntry
    );

typedef BOOL (WINAPI* _DnsFlushResolverCache)(
    VOID
    );

typedef BOOL (WINAPI* _DnsFlushResolverCacheEntry)(
    _In_ LPCWSTR Name
    );

typedef DNS_STATUS (WINAPI* _DnsQuery)(
    _In_ PCTSTR Name,
    _In_ USHORT Type,
    _In_ ULONG Options,
    _Inout_opt_ PVOID Extra,
    _Out_opt_ PDNS_RECORD* QueryResultsSet,
    _Out_opt_ PVOID* Reserved
    );

typedef VOID (WINAPI* _DnsFree)(
    _Inout_ PVOID Data,
    _In_ DNS_FREE_TYPE FreeType
    );

#endif
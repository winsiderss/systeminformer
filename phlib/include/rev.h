/*
 * Process Hacker - 
 *   reverse engineered definitions
 * 
 * Copyright (C) 2010 evilpie
 * Copyright (C) 2010 wj32
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

#ifndef _PH_REV_H
#define _PH_REV_H

// This file consists entirely of reverse-engineered code.

// Service tags

typedef enum _SC_SERVICE_TAG_QUERY_TYPE
{
    ServiceNameFromTagInformation = 1,
    ServiceNamesReferencingModuleInformation,
    ServiceNameTagMappingInformation
} SC_SERVICE_TAG_QUERY_TYPE, *PSC_SERVICE_TAG_QUERY_TYPE;

// ServiceNameFromTagInformation

typedef struct _SC_SERVICE_NAME_FROM_TAG_QUERY
{
    ULONG ProcessId;
    ULONG ServiceTag;
    ULONG Unknown;
    PVOID Buffer;
} SC_SERVICE_NAME_FROM_TAG_QUERY, *PSC_SERVICE_NAME_FROM_TAG_QUERY;

// ServiceNamesReferencingModuleInformation

typedef struct _SC_SERVICE_NAMES_REFERENCING_MODULE_QUERY
{
    ULONG ProcessId;
    PWSTR Module;
    ULONG Count;
    PWSTR *ServiceNames;
} SC_SERVICE_NAMES_REFERENCING_MODULE_QUERY, *PSC_SERVICE_NAMES_REFERENCING_MODULE_QUERY;

// ServiceNameTagMappingInformation

typedef struct _SC_SERVICE_TAG_INFO
{
    ULONG ServiceTag;
    PWSTR ServiceName;
    ULONG Unknown;
} SC_SERVICE_TAG_INFO, *PSC_SERVICE_TAG_INFO;

typedef struct _SC_SERVICE_TAG_LIST
{
    ULONG Count;
    PVOID Unknown1;
    PVOID Unknown2;
    SC_SERVICE_TAG_INFO Services[1];
} SC_SERVICE_TAG_LIST, *PSC_SERVICE_TAG_LIST;

typedef struct _SC_SERVICE_NAME_TAG_MAPPING_QUERY
{
    ULONG Unknown;
    PVOID Buffer;
} SC_SERVICE_NAME_TAG_MAPPING_QUERY, *PSC_SERVICE_NAME_TAG_MAPPING_QUERY;

typedef ULONG (NTAPI *_I_QueryTagInformation)(
    __in PVOID Unknown,
    __in SC_SERVICE_TAG_QUERY_TYPE QueryType,
    __inout PVOID Query
    );

// Winsta

typedef BOOLEAN (NTAPI *_WinStationConnectW)(
    __in HANDLE ServerHandle,
    __in ULONG TargetSessionId,
    __in ULONG CurrentSessionId, // -1 for current session ID
    __in_opt PWSTR Password,
    __in BOOLEAN Wait
    );

#endif

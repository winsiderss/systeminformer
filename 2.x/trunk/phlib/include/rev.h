/*
 * Process Hacker - 
 *   reverse engineered definitions
 * 
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

#ifndef REV_H
#define REV_H

// This file consists entirely of reverse-engineered code.

// Service tags

typedef enum _SC_SERVICE_TAG_QUERY_TYPE
{
    ServiceNameFromTagInformation = 1,
    ServiceNamesReferencingModuleInformation,
    ServiceNameTagMappingInformation
} SC_SERVICE_TAG_QUERY_TYPE, *PSC_SERVICE_TAG_QUERY_TYPE;

typedef struct _SC_SERVICE_TAG_QUERY
{
    ULONG ProcessId;
    ULONG ServiceTag;
    ULONG Unknown;
    PVOID Buffer;
} SC_SERVICE_TAG_QUERY, *PSC_SERVICE_TAG_QUERY;

typedef ULONG (NTAPI *_I_QueryTagInformation)(
    __in PVOID Unknown,
    __in SC_SERVICE_TAG_QUERY_TYPE QueryType,
    __inout PSC_SERVICE_TAG_QUERY Query
    );

#endif

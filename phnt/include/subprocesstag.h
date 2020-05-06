/*
 * Process Hacker -
 *   Subprocess tag information
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

#ifndef _SUBPROCESSTAG_H
#define _SUBPROCESSTAG_H

typedef enum _TAG_INFO_LEVEL
{
    eTagInfoLevelNameFromTag = 1, // TAG_INFO_NAME_FROM_TAG
    eTagInfoLevelNamesReferencingModule, // TAG_INFO_NAMES_REFERENCING_MODULE
    eTagInfoLevelNameTagMapping, // TAG_INFO_NAME_TAG_MAPPING
    eTagInfoLevelMax
} TAG_INFO_LEVEL;

typedef enum _TAG_TYPE
{
    eTagTypeService = 1,
    eTagTypeMax
} TAG_TYPE;

typedef struct _TAG_INFO_NAME_FROM_TAG_IN_PARAMS
{
    ULONG dwPid;
    ULONG dwTag;
} TAG_INFO_NAME_FROM_TAG_IN_PARAMS, *PTAG_INFO_NAME_FROM_TAG_IN_PARAMS;

typedef struct _TAG_INFO_NAME_FROM_TAG_OUT_PARAMS
{
    ULONG eTagType;
    PWSTR pszName;
} TAG_INFO_NAME_FROM_TAG_OUT_PARAMS, *PTAG_INFO_NAME_FROM_TAG_OUT_PARAMS;

typedef struct _TAG_INFO_NAME_FROM_TAG
{
    TAG_INFO_NAME_FROM_TAG_IN_PARAMS InParams;
    TAG_INFO_NAME_FROM_TAG_OUT_PARAMS OutParams;
} TAG_INFO_NAME_FROM_TAG, *PTAG_INFO_NAME_FROM_TAG;

typedef struct _TAG_INFO_NAMES_REFERENCING_MODULE_IN_PARAMS
{
    ULONG dwPid;
    PWSTR pszModule;
} TAG_INFO_NAMES_REFERENCING_MODULE_IN_PARAMS, *PTAG_INFO_NAMES_REFERENCING_MODULE_IN_PARAMS;

typedef struct _TAG_INFO_NAMES_REFERENCING_MODULE_OUT_PARAMS
{
    ULONG eTagType;
    PWSTR pmszNames;
} TAG_INFO_NAMES_REFERENCING_MODULE_OUT_PARAMS, *PTAG_INFO_NAMES_REFERENCING_MODULE_OUT_PARAMS;

typedef struct _TAG_INFO_NAMES_REFERENCING_MODULE
{
    TAG_INFO_NAMES_REFERENCING_MODULE_IN_PARAMS InParams;
    TAG_INFO_NAMES_REFERENCING_MODULE_OUT_PARAMS OutParams;
} TAG_INFO_NAMES_REFERENCING_MODULE, *PTAG_INFO_NAMES_REFERENCING_MODULE;

typedef struct _TAG_INFO_NAME_TAG_MAPPING_IN_PARAMS
{
    ULONG dwPid;
} TAG_INFO_NAME_TAG_MAPPING_IN_PARAMS, *PTAG_INFO_NAME_TAG_MAPPING_IN_PARAMS;

typedef struct _TAG_INFO_NAME_TAG_MAPPING_ELEMENT
{
    ULONG eTagType;
    ULONG dwTag;
    PWSTR pszName;
    PWSTR pszGroupName;
} TAG_INFO_NAME_TAG_MAPPING_ELEMENT, *PTAG_INFO_NAME_TAG_MAPPING_ELEMENT;

typedef struct _TAG_INFO_NAME_TAG_MAPPING_OUT_PARAMS
{
    ULONG cElements;
    PTAG_INFO_NAME_TAG_MAPPING_ELEMENT pNameTagMappingElements;
} TAG_INFO_NAME_TAG_MAPPING_OUT_PARAMS, *PTAG_INFO_NAME_TAG_MAPPING_OUT_PARAMS;

typedef struct _TAG_INFO_NAME_TAG_MAPPING
{
    TAG_INFO_NAME_TAG_MAPPING_IN_PARAMS InParams;
    PTAG_INFO_NAME_TAG_MAPPING_OUT_PARAMS pOutParams;
} TAG_INFO_NAME_TAG_MAPPING, *PTAG_INFO_NAME_TAG_MAPPING;

_Must_inspect_result_
ULONG
WINAPI
I_QueryTagInformation(
    _In_opt_ PCWSTR MachineName,
    _In_ TAG_INFO_LEVEL InfoLevel,
    _Inout_ PVOID TagInfo
    );

typedef ULONG (WINAPI *PQUERY_TAG_INFORMATION)(
    _In_opt_ PCWSTR MachineName,
    _In_ TAG_INFO_LEVEL InfoLevel,
    _Inout_ PVOID TagInfo
    );

#endif

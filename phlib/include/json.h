/*
 * Process Hacker -
 *   json wrapper
 *
 * Copyright (C) 2017 dmex
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

#ifndef _PH_PHJSON_H
#define _PH_PHJSON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _JSON_ARRAY_LIST_OBJECT
{
    PSTR Key;
    PVOID Entry;
} JSON_ARRAY_LIST_OBJECT, *PJSON_ARRAY_LIST_OBJECT;

PHLIBAPI
PVOID
NTAPI
PhCreateJsonParser(
    _In_ PSTR JsonString
    );

PHLIBAPI
VOID
NTAPI
PhFreeJsonParser(
    _In_ PVOID Object
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetJsonValueAsString(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

PHLIBAPI
INT64
NTAPI
PhGetJsonValueAsLong64(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

PHLIBAPI
PVOID
NTAPI
PhCreateJsonObject(
    VOID
    );

PHLIBAPI
PVOID
NTAPI
PhGetJsonObject(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

PHLIBAPI
INT
NTAPI
PhGetJsonObjectLength(
    _In_ PVOID Object
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetJsonObjectBool(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

PHLIBAPI
VOID
NTAPI
PhAddJsonObject(
    _In_ PVOID Object,
    _In_ PSTR Key,
    _In_ PSTR Value
    );

PHLIBAPI
PVOID
NTAPI
PhCreateJsonArray(
    VOID
    );

PHLIBAPI
VOID
NTAPI
PhAddJsonArrayObject(
    _In_ PVOID Object,
    _In_ PVOID jsonEntry
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetJsonArrayString(
    _In_ PVOID Object
    );

PHLIBAPI
INT64
NTAPI
PhGetJsonArrayLong64(
    _In_ PVOID Object,
    _In_ INT Index
    );

PHLIBAPI
INT
NTAPI
PhGetJsonArrayLength(
    _In_ PVOID Object
    );

PHLIBAPI
PVOID
NTAPI
PhGetJsonArrayIndexObject(
    _In_ PVOID Object,
    _In_ INT Index
    );

PHLIBAPI
PPH_LIST
NTAPI
PhGetJsonObjectAsArrayList(
    _In_ PVOID Object
    );

PHLIBAPI
PVOID
NTAPI
PhLoadJsonObjectFromFile(
    _In_ PWSTR FileName
    );

PHLIBAPI
VOID
NTAPI
PhSaveJsonObjectToFile(
    _In_ PWSTR FileName,
    _In_ PVOID Object
    );

#ifdef __cplusplus
}
#endif

#endif

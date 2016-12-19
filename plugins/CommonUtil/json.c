/*
 * Process Hacker Plugins - 
 *   CommonUtil Plugin
 *
 * Copyright (C) 2016 dmex
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
 *
 */

#include "main.h"
#include "json-c\json.h"

json_object_ptr json_get_object(
    _In_ json_object_ptr rootObj, 
    _In_ const PSTR key
    )
{
    json_object_ptr returnObj;

    if (json_object_object_get_ex(rootObj, key, &returnObj))
    {
        return returnObj;
    }

    return NULL;
}

PVOID UtilCreateJsonParser(
    _In_ PSTR JsonString
    )
{
    return json_tokener_parse(JsonString);
}

VOID UtilCleanupJsonParser(
    _In_ PVOID Object
    )
{
    json_object_put(Object);
}

PSTR UtilGetJsonValueAsString(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    return json_object_get_string(json_get_object(Object, Key));
}

int64_t UtilGetJsonValueAsUlong(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    return json_object_get_int64(json_get_object(Object, Key));
}

PVOID UtilCreateJsonObject(
    VOID
    )
{
    return json_object_new_object();
}

PVOID UtilGetJsonObject(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    return json_get_object(Object, Key);
}

VOID UtilJsonAddObject(
    _In_ PVOID Object,
    _In_ PSTR Key,
    _In_ PSTR Value
    )
{
    json_object_object_add(Object, Key, json_object_new_string(Value));
}

PVOID UtilCreateJsonArray(
    VOID
    )
{
    return json_object_new_array();
}

VOID UtilAddJsonArray(
    _In_ PVOID Object,
    _In_ PVOID jsonEntry
    )
{
    json_object_array_add(Object, jsonEntry);
}

PSTR UtilGetJsonArrayString(
    _In_ PVOID Object
    )
{
    return _strdup(json_object_to_json_string(Object));
}

INT UtilGetArrayLength(
    _In_ PVOID Object
    )
{
    return json_object_array_length(Object);
}

PVOID UtilGetObjectArrayIndex(
    _In_ PVOID Object,
    _In_ INT Index
    )
{
    return json_object_array_get_idx(Object, Index);
}

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

#include <ph.h>
#include <phbasesup.h>
#include <phutil.h>

#include "jsonc\json.h"
#include <json.h>

static json_object_ptr json_get_object(
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

PVOID PhCreateJsonParser(
    _In_ PSTR JsonString
    )
{
    return json_tokener_parse(JsonString);
}

VOID PhFreeJsonParser(
    _In_ PVOID Object
    )
{
    json_object_put(Object);
}

PPH_STRING PhGetJsonValueAsString(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    PSTR value;

    if (value = json_object_get_string(json_get_object(Object, Key)))
        return PhConvertUtf8ToUtf16(value);
    else
        return NULL;
}

INT64 PhGetJsonValueAsLong64(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    return json_object_get_int64(json_get_object(Object, Key));
}

PVOID PhCreateJsonObject(
    VOID
    )
{
    return json_object_new_object();
}

PVOID PhGetJsonObject(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    return json_get_object(Object, Key);
}

INT PhGetJsonObjectLength(
    _In_ PVOID Object
    )
{
    return json_object_object_length(Object);
}

BOOLEAN PhGetJsonObjectBool(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    return json_object_get_boolean(json_get_object(Object, Key)) == TRUE;
}

VOID PhAddJsonObject(
    _In_ PVOID Object,
    _In_ PSTR Key,
    _In_ PSTR Value
    )
{
    json_object_object_add(Object, Key, json_object_new_string(Value));
}

PVOID PhCreateJsonArray(
    VOID
    )
{
    return json_object_new_array();
}

VOID PhAddJsonArrayObject(
    _In_ PVOID Object,
    _In_ PVOID jsonEntry
    )
{
    json_object_array_add(Object, jsonEntry);
}

PPH_STRING PhGetJsonArrayString(
    _In_ PVOID Object
    )
{
    PSTR value;

    if (value = json_object_to_json_string(Object))
        return PhConvertUtf8ToUtf16(value);
    else
        return NULL;
}

INT64 PhGetJsonArrayLong64(
    _In_ PVOID Object,
    _In_ INT Index
    )
{
    return json_object_get_int64(json_object_array_get_idx(Object, Index));
}

INT PhGetJsonArrayLength(
    _In_ PVOID Object
    )
{
    return json_object_array_length(Object);
}

PVOID PhGetJsonArrayIndexObject(
    _In_ PVOID Object,
    _In_ INT Index
    )
{
    return json_object_array_get_idx(Object, Index);
}

PPH_LIST PhGetJsonObjectAsArrayList(
    _In_ PVOID Object
    )
{
    PPH_LIST listArray;
    json_object_iter json_array_ptr;

    listArray = PhCreateList(1);

    json_object_object_foreachC(Object, json_array_ptr)
    {
        PJSON_ARRAY_LIST_OBJECT object;
        
        object = PhAllocate(sizeof(JSON_ARRAY_LIST_OBJECT));
        memset(object, 0, sizeof(JSON_ARRAY_LIST_OBJECT));

        object->Key = json_array_ptr.key;
        object->Entry = json_array_ptr.val;

        PhAddItemList(listArray, object);
    }

    return listArray;
}

PVOID PhLoadJsonObjectFromFile(
    _In_ PWSTR FileName
    )
{
    return json_object_from_file(FileName);
}

VOID PhSaveJsonObjectToFile(
    _In_ PWSTR FileName,
    _In_ PVOID Object
    )
{
    json_object_to_file(FileName, Object);
}
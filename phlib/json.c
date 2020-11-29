/*
 * Process Hacker -
 *   json and xml wrapper
 *
 * Copyright (C) 2017-2020 dmex
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

#include "..\tools\thirdparty\jsonc\json.h"
#include "..\tools\thirdparty\mxml\mxml.h"

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
    PCSTR value;

    if (value = json_object_get_string(json_get_object(Object, Key)))
        return PhConvertUtf8ToUtf16((PSTR)value);
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

PH_JSON_OBJECT_TYPE PhGetJsonObjectType(
    _In_ PVOID Object
    )
{
    switch (json_object_get_type(Object))
    {
    case json_type_null:
        return PH_JSON_OBJECT_TYPE_NULL;
    case json_type_boolean:
        return PH_JSON_OBJECT_TYPE_BOOLEAN;
    case json_type_double:
        return PH_JSON_OBJECT_TYPE_DOUBLE;
    case json_type_int:
        return PH_JSON_OBJECT_TYPE_INT;
    case json_type_object:
        return PH_JSON_OBJECT_TYPE_OBJECT;
    case json_type_array:
        return PH_JSON_OBJECT_TYPE_ARRAY;
    case json_type_string:
        return PH_JSON_OBJECT_TYPE_STRING;
    }

    return PH_JSON_OBJECT_TYPE_UNKNOWN;
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

VOID PhAddJsonObjectInt64(
    _In_ PVOID Object,
    _In_ PSTR Key,
    _In_ ULONGLONG Value
    )
{
    json_object_object_add(Object, Key, json_object_new_int64(Value));
}

VOID PhAddJsonObjectDouble(
    _In_ PVOID Object,
    _In_ PSTR Key,
    _In_ DOUBLE Value
    )
{
    json_object_object_add(Object, Key, json_object_new_double(Value));
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
    PCSTR value;

    if (value = json_object_get_string(Object)) // json_object_to_json_string_ext(Object, JSON_C_TO_STRING_PLAIN))
        return PhConvertUtf8ToUtf16((PSTR)value);
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
    return (INT)json_object_array_length(Object);
}

PVOID PhGetJsonArrayIndexObject(
    _In_ PVOID Object,
    _In_ INT Index
    )
{
    return json_object_array_get_idx(Object, Index);
}

PVOID PhGetJsonObjectAsArrayList(
    _In_ PVOID Object
    )
{
    PPH_LIST listArray;
    json_object_iter json_array_ptr;

    listArray = PhCreateList(1);

    json_object_object_foreachC(Object, json_array_ptr)
    {
        PJSON_ARRAY_LIST_OBJECT object;
        
        object = PhAllocateZero(sizeof(JSON_ARRAY_LIST_OBJECT));
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

// XML support

NTSTATUS PhLoadXmlObjectFromFile(
    _In_ PWSTR FileName,
    _Out_opt_ PVOID* XmlRootObject
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    mxml_node_t* currentNode;

    status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    if (NT_SUCCESS(PhGetFileSize(fileHandle, &fileSize)) && fileSize.QuadPart == 0)
    {
        // A blank file is OK.
        NtClose(fileHandle);
        return STATUS_END_OF_FILE;
    }

    currentNode = mxmlLoadFd(
        NULL,
        fileHandle,
        MXML_OPAQUE_CALLBACK
        );

    NtClose(fileHandle);

    if (currentNode)
    {
        if (mxmlGetType(currentNode) == MXML_ELEMENT)
        {
            if (XmlRootObject)
                *XmlRootObject = currentNode;

            return STATUS_SUCCESS;
        }

        mxmlDelete(currentNode);
    }

    return STATUS_FILE_CORRUPT_ERROR;
}

NTSTATUS PhSaveXmlObjectToFile(
    _In_ PWSTR FileName,
    _In_ PVOID XmlRootObject,
    _In_opt_ PVOID XmlSaveCallback
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    PPH_STRING fullPath;
    ULONG indexOfFileName;

    // Create the directory if it does not exist.  
    if (fullPath = PhGetFullPath(FileName, &indexOfFileName))
    {
        if (indexOfFileName != ULONG_MAX)
        {
            PPH_STRING directoryName;

            directoryName = PhSubstring(fullPath, 0, indexOfFileName);
            status = PhCreateDirectory(directoryName);

            if (!NT_SUCCESS(status))
            {
                PhDereferenceObject(directoryName);
                PhDereferenceObject(fullPath);
                return status;
            }

            PhDereferenceObject(directoryName);
        }

        PhDereferenceObject(fullPath);
    }  

    status = PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    if (mxmlSaveFd(XmlRootObject, fileHandle, XmlSaveCallback) == -1)
    {
        status = STATUS_UNSUCCESSFUL;
    }

    NtClose(fileHandle);

    return status;
}

PVOID PhCreateXmlNode(
    _In_opt_ PVOID ParentNode,
    _In_ PSTR Name
    )
{
    return mxmlNewElement(ParentNode, Name);
}

PVOID PhCreateXmlOpaqueNode(
    _In_opt_ PVOID ParentNode,
    _In_ PSTR Value
    )
{
    return mxmlNewOpaque(ParentNode, Value);
}

VOID PhFreeXmlObject(
    _In_ PVOID XmlRootObject
    )
{
    mxmlDelete(XmlRootObject);
}

PVOID PhGetXmlNodeFirstChild(
    _In_ PVOID XmlNodeObject
    )
{
    return mxmlGetFirstChild(XmlNodeObject);
}

PVOID PhGetXmlNodeNextChild(
    _In_ PVOID XmlNodeObject
    )
{
    return mxmlGetNextSibling(XmlNodeObject);
}

PPH_STRING PhGetOpaqueXmlNodeText(
    _In_ PVOID XmlNodeObject
    )
{
    PCSTR string;

    if (string = mxmlGetOpaque(XmlNodeObject))
    {
        return PhConvertUtf8ToUtf16((PSTR)string);
    }
    else
    {
        return PhReferenceEmptyString();
    }
}

PSTR PhGetXmlNodeElementText(
    _In_ PVOID XmlNodeObject
    )
{
    return (PSTR)mxmlGetElement(XmlNodeObject);
}

PPH_STRING PhGetXmlNodeAttributeText(
    _In_ PVOID XmlNodeObject,
    _In_ PSTR AttributeName
    )
{
    PCSTR string;

    if (string = mxmlElementGetAttr(XmlNodeObject, AttributeName))
    {
        return PhConvertUtf8ToUtf16((PSTR)string);
    }

    return NULL;
}

PSTR PhGetXmlNodeAttributeByIndex(
    _In_ PVOID XmlNodeObject,
    _In_ INT Index,
    _Out_ PSTR* AttributeName
    )
{
    return (PSTR)mxmlElementGetAttrByIndex(XmlNodeObject, Index, AttributeName);
}

VOID PhSetXmlNodeAttributeText(
    _In_ PVOID XmlNodeObject,
    _In_ PSTR Name,
    _In_ PSTR Value
    )
{
    mxmlElementSetAttr(XmlNodeObject, Name, Value);
}

INT PhGetXmlNodeAttributeCount(
    _In_ PVOID XmlNodeObject
    )
{
    return mxmlElementGetAttrCount(XmlNodeObject);
}

typedef BOOLEAN (NTAPI* PPH_ENUM_XML_NODE_CALLBACK)(
    _In_ PVOID XmlNodeObject,
    _In_opt_ PVOID Context
    );

BOOLEAN PhEnumXmlNode(
    _In_ PVOID XmlNodeObject,
    _In_ PPH_ENUM_XML_NODE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PVOID currentNode;

    currentNode = PhGetXmlNodeFirstChild(XmlNodeObject);

    while (currentNode)
    {
        if (Callback(currentNode, Context))
            break;

        currentNode = PhGetXmlNodeNextChild(currentNode);
    }

    return TRUE;
}

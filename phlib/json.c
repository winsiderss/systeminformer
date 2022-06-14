/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2022
 *
 */

#include <ph.h>
#include <phbasesup.h>
#include <phutil.h>
#include <json.h>

#include "..\tools\thirdparty\jsonc\json.h"
#include "..\tools\thirdparty\mxml\mxml.h"

static json_object_ptr json_get_object(
    _In_ json_object_ptr rootObj, 
    _In_ PSTR key
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

PVOID PhCreateJsonParserEx(
    _In_ PVOID JsonString,
    _In_ BOOLEAN Unicode
    )
{
    json_tokener* tokenerObject;
    json_object_ptr jsonObject;

    if (Unicode)
    {
        PPH_STRING jsonStringUtf16 = JsonString;
        PPH_BYTES jsonStringUtf8;

        if (jsonStringUtf16->Length / sizeof(WCHAR) >= INT32_MAX)
            return NULL;
        if (!(tokenerObject = json_tokener_new()))
            return NULL;

        json_tokener_set_flags(
            tokenerObject,
            JSON_TOKENER_STRICT | JSON_TOKENER_VALIDATE_UTF8
            );

        jsonStringUtf8 = PhConvertUtf16ToUtf8Ex(
            jsonStringUtf16->Buffer,
            jsonStringUtf16->Length
            );
        jsonObject = json_tokener_parse_ex(
            tokenerObject,
            jsonStringUtf8->Buffer,
            (INT)jsonStringUtf8->Length
            );
        PhDereferenceObject(jsonStringUtf8);
    }
    else
    {
        PPH_BYTES jsonStringUtf8 = JsonString;

        if (jsonStringUtf8->Length >= INT32_MAX)
            return NULL;
        if (!(tokenerObject = json_tokener_new()))
            return NULL;

        json_tokener_set_flags(
            tokenerObject,
            JSON_TOKENER_STRICT | JSON_TOKENER_VALIDATE_UTF8
            );

        jsonObject = json_tokener_parse_ex(
            tokenerObject,
            jsonStringUtf8->Buffer,
            (INT)jsonStringUtf8->Length
            );
    }

    if (json_tokener_get_error(tokenerObject) != json_tokener_success)
    {
        json_tokener_free(tokenerObject);
        return NULL;
    }

    json_tokener_free(tokenerObject);

    return jsonObject;
}

VOID PhFreeJsonObject(
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
    PVOID object;
    PCSTR value;
    size_t length;

    if (object = json_get_object(Object, Key))
    {
        if (
            (length = json_object_get_string_len(object)) &&
            (value = json_object_get_string(object))
            )
        {
            return PhConvertUtf8ToUtf16Ex((PSTR)value, length);
        }
    }

    return NULL;
}

LONGLONG PhGetJsonValueAsInt64(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    return json_object_get_int64(json_get_object(Object, Key));
}

ULONGLONG PhGetJsonValueAsUInt64(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    return json_object_get_uint64(json_get_object(Object, Key));
}

PVOID PhCreateJsonObject(
    VOID
    )
{
    return json_object_new_object();
}

PVOID PhCreateJsonStringObject(
    _In_ PSTR Value
    )
{
    return json_object_new_string(Value);
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

VOID PhAddJsonObjectValue(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PVOID Value
    )
{
    json_object_object_add_ex(Object, Key, Value, JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_ADD_CONSTANT_KEY);
}

VOID PhAddJsonObject(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PSTR Value
    )
{
    json_object_object_add_ex(Object, Key, json_object_new_string(Value), JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_ADD_CONSTANT_KEY);
}

VOID PhAddJsonObject2(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PSTR Value,
    _In_ SIZE_T Length
    )
{
    PVOID string = json_object_new_string_len(Value, (UINT32)Length);

    json_object_object_add_ex(Object, Key, string, JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_ADD_CONSTANT_KEY);
}

VOID PhAddJsonObjectInt64(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ LONGLONG Value
    )
{
    json_object_object_add_ex(Object, Key, json_object_new_int64(Value), JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_ADD_CONSTANT_KEY);
}

VOID PhAddJsonObjectUInt64(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONGLONG Value
    )
{
    json_object_object_add_ex(Object, Key, json_object_new_uint64(Value), JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_ADD_CONSTANT_KEY);
}

VOID PhAddJsonObjectDouble(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ DOUBLE Value
    )
{
    json_object_object_add_ex(Object, Key, json_object_new_double(Value), JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_ADD_CONSTANT_KEY);
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

PVOID PhGetJsonArrayString(
    _In_ PVOID Object,
    _In_ BOOLEAN Unicode
    )
{
    PCSTR value;
    size_t length;

    if (value = json_object_to_json_string_length(Object, JSON_C_TO_STRING_PLAIN, &length)) // json_object_get_string(Object))
    {
        if (Unicode)
            return PhConvertUtf8ToUtf16Ex((PSTR)value, length);
        else
            return PhCreateBytesEx((PSTR)value, length);
    }

    return NULL;
}

INT64 PhGetJsonArrayLong64(
    _In_ PVOID Object,
    _In_ ULONG Index
    )
{
    return json_object_get_int64(json_object_array_get_idx(Object, Index));
}

ULONG PhGetJsonArrayLength(
    _In_ PVOID Object
    )
{
    return (ULONG)json_object_array_length(Object);
}

PVOID PhGetJsonArrayIndexObject(
    _In_ PVOID Object,
    _In_ ULONG Index
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

PVOID PhLoadXmlObjectFromString(
    _In_ PSTR String
    )
{
    mxml_node_t* currentNode;

    if (currentNode = mxmlLoadString(
        NULL,
        String,
        MXML_OPAQUE_CALLBACK
        ))
    {
        if (mxmlGetType(currentNode) == MXML_ELEMENT)
        {
            return currentNode;
        }

        mxmlDelete(currentNode);
    }

    return NULL;
}

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

VOID PhFreeXmlObject(
    _In_ PVOID XmlRootObject
    )
{
    mxmlDelete(XmlRootObject);
}

PVOID PhGetXmlObject(
    _In_ PVOID XmlNodeObject,
    _In_ PSTR Path
    )
{
    mxml_node_t* currentNode;
    mxml_node_t* realNode;

    if (currentNode = mxmlFindPath(XmlNodeObject, Path))
    {
        if (realNode = mxmlGetParent(currentNode))
            return realNode;

        return currentNode;
    }

    return NULL;
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

PVOID PhFindXmlObject(
    _In_ PVOID XmlNodeObject,
    _In_opt_ PVOID XmlTopObject,
    _In_opt_ PSTR Element,
    _In_opt_ PSTR Attribute,
    _In_opt_ PSTR Value
    )
{
    return mxmlFindElement(XmlNodeObject, XmlTopObject, Element, Attribute, Value, MXML_DESCEND);
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

PPH_STRING PhGetXmlNodeOpaqueText(
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

PPH_XML_INTERFACE PhGetXmlInterface(
    _In_ ULONG Version
    )
{
    static PH_XML_INTERFACE PhXmlInterface =
    {
        PH_XML_INTERFACE_VERSION,
        PhLoadXmlObjectFromString,
        PhLoadXmlObjectFromFile,
        PhSaveXmlObjectToFile,
        PhFreeXmlObject,
        PhGetXmlObject,
        PhCreateXmlNode,
        PhCreateXmlOpaqueNode,
        PhFindXmlObject,
        PhGetXmlNodeFirstChild,
        PhGetXmlNodeNextChild,
        PhGetXmlNodeOpaqueText,
        PhGetXmlNodeElementText,
        PhGetXmlNodeAttributeText,
        PhGetXmlNodeAttributeByIndex,
        PhSetXmlNodeAttributeText,
        PhGetXmlNodeAttributeCount
    };

    if (Version < PH_XML_INTERFACE_VERSION)
        return NULL;

    return &PhXmlInterface;
}

/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2023
 *
 */

#include <ph.h>
#include <phbasesup.h>
#include <phutil.h>
#include <json.h>
#include <thirdparty.h>

static PVOID json_get_object(
    _In_ PVOID Object,
    _In_ PCSTR Key
    )
{
    json_object* returnObj;

    if (json_object_object_get_ex(Object, Key, &returnObj))
    {
        return returnObj;
    }

    return NULL;
}

static NTSTATUS PhJsonErrorToNtStatus(
    _In_ enum json_tokener_error JsonError
    )
{
    switch (JsonError)
    {
    case json_tokener_success:
        return STATUS_SUCCESS;
    case json_tokener_continue:
        return STATUS_MORE_ENTRIES;
    case json_tokener_error_depth:
        return STATUS_PARTIAL_COPY;
    case json_tokener_error_parse_eof:
    case json_tokener_error_parse_unexpected:
    case json_tokener_error_parse_null:
    case json_tokener_error_parse_boolean:
    case json_tokener_error_parse_number:
    case json_tokener_error_parse_array:
    case json_tokener_error_parse_object_key_name:
    case json_tokener_error_parse_object_key_sep:
    case json_tokener_error_parse_object_value_sep:
    case json_tokener_error_parse_string:
    case json_tokener_error_parse_comment:
    case json_tokener_error_parse_utf8_string:
        return STATUS_FAIL_CHECK;
    case json_tokener_error_memory:
        return STATUS_NO_MEMORY;
    case json_tokener_error_size:
        return STATUS_BUFFER_TOO_SMALL;
    }

    return STATUS_UNSUCCESSFUL;
}

PVOID PhCreateJsonParser(
    _In_ PCSTR JsonString
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
    json_object* jsonObject;

    if (Unicode)
    {
        PPH_STRING jsonStringUtf16 = JsonString;
        PPH_BYTES jsonStringUtf8;

        if (jsonStringUtf16->Length / sizeof(WCHAR) >= INT32_MAX)
            return NULL; // STATUS_INVALID_BUFFER_SIZE
        if (!(tokenerObject = json_tokener_new()))
            return NULL; // STATUS_NO_MEMORY

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
            (LONG)jsonStringUtf8->Length
            );
        PhDereferenceObject(jsonStringUtf8);
    }
    else
    {
        PPH_BYTES jsonStringUtf8 = JsonString;

        if (jsonStringUtf8->Length >= INT32_MAX)
            return NULL; // STATUS_INVALID_BUFFER_SIZE
        if (!(tokenerObject = json_tokener_new()))
            return NULL; // STATUS_NO_MEMORY

        json_tokener_set_flags(
            tokenerObject,
            JSON_TOKENER_STRICT | JSON_TOKENER_VALIDATE_UTF8
            );

        jsonObject = json_tokener_parse_ex(
            tokenerObject,
            jsonStringUtf8->Buffer,
            (LONG)jsonStringUtf8->Length
            );
    }

    if (json_tokener_get_error(tokenerObject) != json_tokener_success)
    {
        json_tokener_free(tokenerObject);
        return NULL; // STATUS_UNSUCCESSFUL
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
    _In_ PCSTR Key
    )
{
    PVOID object;
    PCSTR value;
    size_t length;

    if (object = json_get_object(Object, (PCSTR)Key))
    {
        if (
            (length = json_object_get_string_len(object)) &&
            (value = json_object_get_string(object))
            )
        {
            return PhConvertUtf8ToUtf16Ex(value, length);
        }
    }

    return NULL;
}

PPH_STRING PhGetJsonObjectString(
    _In_ PVOID Object
    )
{
    PCSTR value;
    size_t length;

    if (
        (length = json_object_get_string_len(Object)) &&
        (value = json_object_get_string(Object))
        )
    {
        return PhConvertUtf8ToUtf16Ex(value, length);
    }

    return PhReferenceEmptyString();
}

LONGLONG PhGetJsonValueAsInt64(
    _In_ PVOID Object,
    _In_ PCSTR Key
    )
{
    return json_object_get_int64(json_get_object(Object, Key));
}

ULONGLONG PhGetJsonValueAsUInt64(
    _In_ PVOID Object,
    _In_ PCSTR Key
    )
{
    return json_object_get_uint64(json_get_object(Object, Key));
}

ULONG PhGetJsonUInt32Object(
    _In_ PVOID Object
    )
{
    return json_object_get_int(Object);
}

PVOID PhCreateJsonObject(
    VOID
    )
{
    return json_object_new_object();
}

PVOID PhCreateJsonStringObject(
    _In_ PCSTR Value
    )
{
    return json_object_new_string(Value);
}

PVOID PhGetJsonObject(
    _In_ PVOID Object,
    _In_ PCSTR Key
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

LONG PhGetJsonObjectLength(
    _In_ PVOID Object
    )
{
    return json_object_object_length(Object);
}

BOOLEAN PhGetJsonObjectBool(
    _In_ PVOID Object,
    _In_ PCSTR Key
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
    _In_ PCSTR Value
    )
{
    json_object_object_add_ex(Object, Key, json_object_new_string(Value), JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_ADD_CONSTANT_KEY);
}

VOID PhAddJsonObject2(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PCSTR Value,
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
    _In_ PVOID Value
    )
{
    json_object_array_add(Object, Value);
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
            return PhConvertUtf8ToUtf16Ex(value, length);
        else
            return PhCreateBytesEx(value, length);
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

VOID PhEnumJsonArrayObject(
    _In_ PVOID Object,
    _In_ PPH_ENUM_JSON_OBJECT_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    json_object_object_foreach(Object, key, value)
    {
        if (!Callback(Object, key, value, Context))
            break;
    }
}

PVOID PhGetJsonObjectAsArrayList(
    _In_ PVOID Object
    )
{
    PPH_LIST listArray;

    listArray = PhCreateList(1);

    json_object_object_foreach(Object, key, value)
    {
        PJSON_ARRAY_LIST_OBJECT object;

        object = PhAllocateZero(sizeof(JSON_ARRAY_LIST_OBJECT));
        object->Key = key;
        object->Entry = value;

        PhAddItemList(listArray, object);
    }

    return listArray;
}

PVOID PhLoadJsonObjectFromFile(
    _In_ PPH_STRINGREF FileName
    )
{
    PPH_BYTES content;
    PVOID object;

    if (content = PhFileReadAllText(FileName, FALSE))
    {
        object = PhCreateJsonParserEx(content, FALSE);

        PhDereferenceObject(content);
        return object;
    }

    return NULL;
}

static CONST PH_FLAG_MAPPING PhJsonFormatFlagMappings[] =
{
    { PH_JSON_TO_STRING_PLAIN, JSON_C_TO_STRING_PLAIN },
    { PH_JSON_TO_STRING_SPACED, JSON_C_TO_STRING_SPACED },
    { PH_JSON_TO_STRING_PRETTY, JSON_C_TO_STRING_PRETTY },
};

NTSTATUS PhSaveJsonObjectToFile(
    _In_ PPH_STRINGREF FileName,
    _In_ PVOID Object,
    _In_opt_ ULONG Flags
    )
{
    static PH_STRINGREF extension = PH_STRINGREF_INIT(L".backup");
    NTSTATUS status;
    LONG json_flags = 0;
    HANDLE fileHandle = NULL;
    PPH_STRING fileName;
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER allocationSize;
    size_t json_length;
    PCSTR json_string;

    json_flags = 0;
    PhMapFlags1(&json_flags, Flags, PhJsonFormatFlagMappings, RTL_NUMBER_OF(PhJsonFormatFlagMappings));

    json_string = json_object_to_json_string_length(
        Object,
        json_flags,
        &json_length
        );

    if (json_length == 0)
        return STATUS_UNSUCCESSFUL;

    allocationSize.QuadPart = json_length;

    // Create the directory if it does not exist.

    status = PhCreateDirectoryFullPath(FileName);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Create a temporary filename.

    fileName = PhGetBaseNameChangeExtension(FileName, &extension);

    // Create the temporary file.

    status = PhCreateFileEx(
        &fileHandle,
        &fileName->sr,
        FILE_GENERIC_WRITE | DELETE,
        NULL,
        &allocationSize,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_NONE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    // Cleanup the temporary filename.

    PhDereferenceObject(fileName);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Write the buffer to the temporary file.

    status = NtWriteFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        (PVOID)json_string,
        (ULONG)json_length,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Flush the temporary file.

    PhFlushBuffersFile(fileHandle);

    // Atomically update the target file:
    // https://learn.microsoft.com/en-us/windows/win32/fileio/deprecation-of-txf#applications-updating-a-single-file-with-document-like-data

    status = PhSetFileRename(
        fileHandle,
        NULL,
        TRUE,
        FileName
        );

CleanupExit:
    if (fileHandle)
    {
        NtClose(fileHandle);
    }

    json_object_put((struct json_object*)json_string);

    return status;
}

// XML support

PVOID PhLoadXmlObjectFromString(
    _In_ PCSTR String
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
    _In_ PPH_STRINGREF FileName,
    _Out_opt_ PVOID* XmlRootObject
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    mxml_node_t* currentNode;

    status = PhCreateFile(
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
    _In_ PPH_STRINGREF FileName,
    _In_ PVOID XmlRootObject,
    _In_opt_ PVOID XmlSaveCallback
    )
{
    static PH_STRINGREF extension = PH_STRINGREF_INIT(L".backup");
    NTSTATUS status;
    PPH_STRING fileName;
    HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER allocationSize;
    LONG xml_length;
    PSTR xml_buffer;

    xml_length = mxmlSaveString(XmlRootObject, NULL, 0, XmlSaveCallback);

    if (xml_length == 0)
        return STATUS_UNSUCCESSFUL;

    xml_buffer = PhAllocateSafe(xml_length);

    if (!xml_buffer)
        return STATUS_UNSUCCESSFUL;

    xml_length = mxmlSaveString(
        XmlRootObject,
        xml_buffer,
        xml_length,
        XmlSaveCallback
        );

    if (xml_length == 0)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    allocationSize.QuadPart = xml_length;

    // Create the directory if it does not exist.

    status = PhCreateDirectoryFullPath(FileName);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Create a temporary filename.

    fileName = PhGetBaseNameChangeExtension(FileName, &extension);

    // Create the temporary file.

    status = PhCreateFileEx(
        &fileHandle,
        &fileName->sr,
        FILE_GENERIC_WRITE | DELETE,
        NULL,
        &allocationSize,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_NONE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    // Cleanup the temporary filename.

    PhDereferenceObject(fileName);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Write the buffer to the temporary file.

    status = NtWriteFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        (PVOID)xml_buffer,
        (ULONG)xml_length,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Flush the temporary file.

    PhFlushBuffersFile(fileHandle);

    // Atomically update the target file:
    // https://learn.microsoft.com/en-us/windows/win32/fileio/deprecation-of-txf#applications-updating-a-single-file-with-document-like-data

    status = PhSetFileRename(
        fileHandle,
        NULL,
        TRUE,
        FileName
        );

CleanupExit:
    if (fileHandle)
    {
        NtClose(fileHandle);
    }

    if (xml_buffer)
    {
        PhFree(xml_buffer);
    }

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
    _In_ PCSTR Path
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
    _In_ PCSTR Name
    )
{
    return mxmlNewElement(ParentNode, Name);
}

PVOID PhCreateXmlOpaqueNode(
    _In_opt_ PVOID ParentNode,
    _In_ PCSTR Value
    )
{
    return mxmlNewOpaque(ParentNode, Value);
}

PVOID PhFindXmlObject(
    _In_ PVOID XmlNodeObject,
    _In_opt_ PVOID XmlTopObject,
    _In_opt_ PCSTR Element,
    _In_opt_ PCSTR Attribute,
    _In_opt_ PCSTR Value
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
        return PhConvertUtf8ToUtf16(string);
    }
    else
    {
        return PhReferenceEmptyString();
    }
}

PCSTR PhGetXmlNodeElementText(
    _In_ PVOID XmlNodeObject
    )
{
    return mxmlGetElement(XmlNodeObject);
}

PCSTR PhGetXmlNodeCDATAText(
    _In_ PVOID XmlNodeObject
    )
{
    return mxmlGetCDATA(XmlNodeObject);
}

PPH_STRING PhGetXmlNodeAttributeText(
    _In_ PVOID XmlNodeObject,
    _In_ PCSTR AttributeName
    )
{
    PCSTR string;

    if (string = mxmlElementGetAttr(XmlNodeObject, AttributeName))
    {
        return PhConvertUtf8ToUtf16(string);
    }

    return NULL;
}

PCSTR PhGetXmlNodeAttributeByIndex(
    _In_ PVOID XmlNodeObject,
    _In_ LONG Index,
    _Out_ PCSTR* AttributeName
    )
{
    return mxmlElementGetAttrByIndex(XmlNodeObject, Index, AttributeName);
}

VOID PhSetXmlNodeAttributeText(
    _In_ PVOID XmlNodeObject,
    _In_ PCSTR Name,
    _In_ PCSTR Value
    )
{
    mxmlElementSetAttr(XmlNodeObject, Name, Value);
}

LONG PhGetXmlNodeAttributeCount(
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

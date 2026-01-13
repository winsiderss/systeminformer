/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017
 *
 */

#ifndef _PH_PHJSON_H
#define _PH_PHJSON_H

EXTERN_C_START

typedef struct _JSON_ARRAY_LIST_OBJECT
{
    PCSTR Key;
    PVOID Entry;
} JSON_ARRAY_LIST_OBJECT, *PJSON_ARRAY_LIST_OBJECT;

PHLIBAPI
NTSTATUS
NTAPI
PhCreateJsonParser(
    _Out_ PVOID* JsonObject,
    _In_ PCSTR JsonString
    );

PHLIBAPI
NTSTATUS
NTAPI
PhCreateJsonParserEx(
    _Out_ PVOID* JsonObject,
    _In_ PVOID JsonString,
    _In_ BOOLEAN Unicode
    );

PHLIBAPI
VOID
NTAPI
PhFreeJsonObject(
    _In_ PVOID Object
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetJsonValueAsString(
    _In_ PVOID Object,
    _In_ PCSTR Key
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetJsonObjectString(
    _In_ PVOID Object
    );

PHLIBAPI
LONGLONG
NTAPI
PhGetJsonValueAsInt64(
    _In_ PVOID Object,
    _In_ PCSTR Key
    );

PHLIBAPI
ULONGLONG
NTAPI
PhGetJsonValueAsUInt64(
    _In_ PVOID Object,
    _In_ PCSTR Key
    );

FORCEINLINE
ULONG
PhGetJsonValueAsUlong(
    _In_ PVOID Object,
    _In_ PCSTR Key
    )
{
    return (ULONG)PhGetJsonValueAsUInt64(Object, Key);
}

PHLIBAPI
ULONG
NTAPI
PhGetJsonUInt32Object(
    _In_ PVOID Object
    );

PHLIBAPI
ULONGLONG
NTAPI
PhGetJsonUInt64Object(
    _In_ PVOID Object
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
    _In_ PCSTR Key
    );

typedef enum _PH_JSON_OBJECT_TYPE
{
    PH_JSON_OBJECT_TYPE_NULL,
    PH_JSON_OBJECT_TYPE_BOOLEAN,
    PH_JSON_OBJECT_TYPE_DOUBLE,
    PH_JSON_OBJECT_TYPE_INT,
    PH_JSON_OBJECT_TYPE_OBJECT,
    PH_JSON_OBJECT_TYPE_ARRAY,
    PH_JSON_OBJECT_TYPE_STRING,
    PH_JSON_OBJECT_TYPE_UNKNOWN
} PH_JSON_OBJECT_TYPE;

PHLIBAPI
PH_JSON_OBJECT_TYPE
NTAPI
PhGetJsonObjectType(
    _In_ PVOID Object
    );

PHLIBAPI
LONG
NTAPI
PhGetJsonObjectLength(
    _In_ PVOID Object
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetJsonObjectBool(
    _In_ PVOID Object,
    _In_ PCSTR Key
    );

PHLIBAPI
VOID
NTAPI
PhAddJsonObjectValue(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PVOID Value
    );

PHLIBAPI
VOID
NTAPI
PhAddJsonObject(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PCSTR Value
    );

PHLIBAPI
VOID
NTAPI
PhAddJsonObject2(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PCSTR Value,
    _In_ SIZE_T Length
    );

PHLIBAPI
VOID
NTAPI
PhAddJsonObjectUtf8(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PPH_BYTES String
    );

PHLIBAPI
VOID
NTAPI
PhAddJsonObjectInt64(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ LONGLONG Value
    );

PHLIBAPI
VOID
NTAPI
PhAddJsonObjectUInt64(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONGLONG Value
    );

PHLIBAPI
VOID
NTAPI
PhAddJsonObjectDouble(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ DOUBLE Value
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
    _In_ PVOID Value
    );

PHLIBAPI
PVOID
NTAPI
PhGetJsonArrayString(
    _In_ PVOID Object,
    _In_ BOOLEAN Unicode
    );

PHLIBAPI
INT64
NTAPI
PhGetJsonArrayLong64(
    _In_ PVOID Object,
    _In_ ULONG Index
    );

PHLIBAPI
ULONG
NTAPI
PhGetJsonArrayLength(
    _In_ PVOID Object
    );

PHLIBAPI
PVOID
NTAPI
PhGetJsonArrayIndexObject(
    _In_ PVOID Object,
    _In_ ULONG Index
    );

typedef BOOLEAN (NTAPI* PPH_ENUM_JSON_OBJECT_CALLBACK)(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PVOID Value,
    _In_opt_ PVOID Context
    );

VOID PhEnumJsonArrayObject(
    _In_ PVOID Object,
    _In_ PPH_ENUM_JSON_OBJECT_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
PVOID
NTAPI
PhGetJsonObjectAsArrayList(
    _In_ PVOID Object
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadJsonObjectFromFile(
    _Out_ PVOID* JsonObject,
    _In_ PCPH_STRINGREF FileName
    );

#define PH_JSON_TO_STRING_PLAIN 0x0001
#define PH_JSON_TO_STRING_SPACED 0x0002
#define PH_JSON_TO_STRING_PRETTY 0x0004

PHLIBAPI
BOOLEAN
NTAPI
PhJsonObjectToString(
    _In_ PPH_BYTES_BUILDER StringBuilder,
    _In_ PVOID Object,
    _In_ ULONG Level,
    _In_ ULONG Flags
    );

PHLIBAPI
PPH_BYTES
NTAPI
PhJsonObjectToJsonString(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSaveJsonObjectToFile(
    _In_ PCPH_STRINGREF FileName,
    _In_ PVOID Object,
    _In_opt_ ULONG Flags
    );

// XML

PHLIBAPI
PVOID
NTAPI
PhLoadXmlObjectFromString(
    _In_ PCSTR String
    );

PHLIBAPI
PVOID
NTAPI
PhLoadXmlObjectFromString2(
    _In_ PCSTR String
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadXmlObjectFromFile(
    _In_ PCPH_STRINGREF FileName,
    _Out_opt_ PVOID* XmlRootObject
    );

PHLIBAPI
NTSTATUS
NTAPI
PhSaveXmlObjectToFile(
    _In_ PCPH_STRINGREF FileName,
    _In_ PVOID XmlRootObject,
    _In_opt_ PVOID XmlSaveCallback
    );

PHLIBAPI
VOID
NTAPI
PhFreeXmlObject(
    _In_ PVOID XmlRootObject
    );

PHLIBAPI
PVOID
NTAPI
PhGetXmlObject(
    _In_ PVOID XmlNodeObject,
    _In_ PCSTR Path
    );

PHLIBAPI
PVOID
NTAPI
PhFindXmlObject(
    _In_ PVOID XmlNodeObject,
    _In_opt_ PVOID XmlTopObject,
    _In_opt_ PCSTR Element,
    _In_opt_ PCSTR Attribute,
    _In_opt_ PCSTR Value
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetXmlNodeOpaqueText(
    _In_ PVOID XmlNodeObject
    );

PHLIBAPI
PCSTR
NTAPI
PhGetXmlNodeElementText(
    _In_ PVOID XmlNodeObject
    );

PHLIBAPI
PCSTR
NTAPI
PhGetXmlNodeCDATAText(
    _In_ PVOID XmlNodeObject
    );

PHLIBAPI
PPH_STRING
NTAPI
PhGetXmlNodeAttributeText(
    _In_ PVOID XmlNodeObject,
    _In_ PCSTR AttributeName
    );

PHLIBAPI
PCSTR
NTAPI
PhGetXmlNodeAttributeByIndex(
    _In_ PVOID XmlNodeObject,
    _In_ SIZE_T Index,
    _Out_ PCSTR* AttributeName
    );

PHLIBAPI
VOID
NTAPI
PhSetXmlNodeAttributeText(
    _In_ PVOID XmlNodeObject,
    _In_ PCSTR Name,
    _In_ PCSTR Value
    );

PHLIBAPI
SIZE_T
NTAPI
PhGetXmlNodeAttributeCount(
    _In_ PVOID XmlNodeObject
    );

PHLIBAPI
PVOID
NTAPI
PhGetXmlNodeFirstChild(
    _In_ PVOID XmlNodeObject
    );

PHLIBAPI
PVOID
NTAPI
PhGetXmlNodeNextChild(
    _In_ PVOID XmlNodeObject
    );

PHLIBAPI
PVOID
NTAPI
PhCreateXmlNode(
    _In_opt_ PVOID ParentNode,
    _In_ PCSTR Name
    );

PHLIBAPI
PVOID
NTAPI
PhCreateXmlOpaqueNode(
    _In_opt_ PVOID ParentNode,
    _In_ PCSTR Value
    );

typedef PVOID (NTAPI* PH_XML_LOAD_OBJECT_FROM_STRING)(
    _In_ PCSTR String
    );

typedef NTSTATUS (NTAPI* PH_XML_LOAD_OBJECT_FROM_FILE)(
    _In_ PCPH_STRINGREF FileName,
    _Out_opt_ PVOID* XmlRootNode
    );

typedef NTSTATUS (NTAPI* PH_XML_SAVE_OBJECT_TO_FILE)(
    _In_ PCPH_STRINGREF FileName,
    _In_ PVOID XmlRootObject,
    _In_opt_ PVOID XmlSaveCallback
    );

typedef VOID (NTAPI* PH_XML_FREE_OBJECT)(
    _In_ PVOID XmlRootObject
    );

typedef PVOID (NTAPI* PH_XML_GET_OBJECT)(
    _In_ PVOID XmlNodeObject,
    _In_ PCSTR Path
    );

typedef PVOID (NTAPI* PH_XML_CREATE_NODE)(
    _In_opt_ PVOID ParentNode,
    _In_ PCSTR Name
    );

typedef PVOID (NTAPI* PH_XML_CREATE_OPAQUE_NODE)(
    _In_opt_ PVOID ParentNode,
    _In_ PCSTR Value
    );

typedef PVOID (NTAPI* PH_XML_FIND_OBJECT)(
    _In_ PVOID XmlNodeObject,
    _In_ PVOID XmlTopObject,
    _In_ PCSTR Element,
    _In_ PCSTR Attribute,
    _In_ PCSTR Value
    );

typedef PVOID (NTAPI* PH_XML_GET_NODE_FIRST_CHILD)(
    _In_ PVOID XmlNodeObject
    );

typedef PVOID (NTAPI* PH_XML_GET_NODE_NEXT_CHILD)(
    _In_ PVOID XmlNodeObject
    );

typedef PPH_STRING (NTAPI* PH_XML_GET_XML_NODE_OPAQUE_TEXT)(
    _In_ PVOID XmlNodeObject
    );

typedef PCSTR (NTAPI* PH_XML_GET_XML_NODE_ELEMENT_TEXT)(
    _In_ PVOID XmlNodeObject
    );

typedef PPH_STRING (NTAPI* PH_XML_GET_XML_NODE_ATTRIBUTE_TEXT)(
    _In_ PVOID XmlNodeObject,
    _In_ PCSTR AttributeName
    );

typedef PCSTR (NTAPI* PH_XML_GET_XML_NODE_ATTRIBUTE_BY_INDEX)(
    _In_ PVOID XmlNodeObject,
    _In_ SIZE_T Index,
    _Out_ PCSTR* AttributeName
    );

typedef VOID (NTAPI* PH_XML_SET_XML_NODE_ATTRIBUTE_TEXT)(
    _In_ PVOID XmlNodeObject,
    _In_ PCSTR Name,
    _In_ PCSTR Value
    );

typedef SIZE_T (NTAPI* PH_XML_GET_XML_NODE_ATTRIBUTE_COUNT)(
    _In_ PVOID XmlNodeObject
    );

typedef struct _PH_XML_INTERFACE
{
    ULONG Version;
    PH_XML_LOAD_OBJECT_FROM_STRING LoadXmlObjectFromString;
    PH_XML_LOAD_OBJECT_FROM_FILE LoadXmlObjectFromFile;
    PH_XML_SAVE_OBJECT_TO_FILE SaveXmlObjectToFile;
    PH_XML_FREE_OBJECT FreeXmlObject;
    PH_XML_GET_OBJECT GetXmlObject;
    PH_XML_CREATE_NODE CreateXmlNode;
    PH_XML_CREATE_OPAQUE_NODE CreateXmlOpaqueNode;
    PH_XML_FIND_OBJECT FindXmlObject;
    PH_XML_GET_NODE_FIRST_CHILD GetXmlNodeFirstChild;
    PH_XML_GET_NODE_NEXT_CHILD GetXmlNodeNextChild;
    PH_XML_GET_XML_NODE_OPAQUE_TEXT GetXmlNodeOpaqueText;
    PH_XML_GET_XML_NODE_ELEMENT_TEXT GetXmlNodeElementText;
    PH_XML_GET_XML_NODE_ATTRIBUTE_TEXT GetXmlNodeAttributeText;
    PH_XML_GET_XML_NODE_ATTRIBUTE_BY_INDEX GetXmlNodeAttributeByIndex;
    PH_XML_SET_XML_NODE_ATTRIBUTE_TEXT SetXmlNodeAttributeText;
    PH_XML_GET_XML_NODE_ATTRIBUTE_COUNT GetXmlNodeAttributeCount;
} PH_XML_INTERFACE, *PPH_XML_INTERFACE;

#define PH_XML_INTERFACE_VERSION 1

PPH_XML_INTERFACE PhGetXmlInterface(
    _In_ ULONG Version
    );

EXTERN_C_END

#endif

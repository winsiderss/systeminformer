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

NTSTATUS PhCreateJsonParser(
    _Out_ PVOID* JsonObject,
    _In_ PCSTR JsonString
    )
{
    if (*JsonObject = json_tokener_parse(JsonString))
        return STATUS_SUCCESS;

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhCreateJsonParserEx(
    _Out_ PVOID* JsonObject,
    _In_ PVOID JsonString,
    _In_ BOOLEAN Unicode
    )
{
    json_tokener* tokenerObject;
    json_object* jsonObject;
    enum json_tokener_error jsonStatus;

    if (Unicode)
    {
        PPH_STRING jsonStringUtf16 = JsonString;
        PPH_BYTES jsonStringUtf8;

        if (jsonStringUtf16->Length / sizeof(WCHAR) >= INT32_MAX)
            return STATUS_INVALID_BUFFER_SIZE;
        if (!(tokenerObject = json_tokener_new()))
            return STATUS_NO_MEMORY;

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
            return STATUS_INVALID_BUFFER_SIZE;
        if (!(tokenerObject = json_tokener_new()))
            return STATUS_NO_MEMORY;

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

    jsonStatus = json_tokener_get_error(tokenerObject);
    if (jsonStatus != json_tokener_success)
    {
        json_tokener_free(tokenerObject);
        return PhJsonErrorToNtStatus(jsonStatus);
    }

    json_tokener_free(tokenerObject);
    *JsonObject = jsonObject;
    return STATUS_SUCCESS;
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

ULONGLONG PhGetJsonUInt64Object(
    _In_ PVOID Object
    )
{
    return json_object_get_uint64(Object);
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

VOID PhAddJsonObjectUtf8(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PPH_BYTES String
    )
{
    PVOID string = json_object_new_string_len(String->Buffer, (UINT32)String->Length);

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
    if (PhGetJsonObjectType(Object) == PH_JSON_OBJECT_TYPE_OBJECT)
    {
        json_object_iter iter;

        json_object_object_foreachC(Object, iter)
        {
            if (!Callback(Object, iter.key, iter.val, Context))
                break;
        }
    }
}

PVOID PhGetJsonObjectAsArrayList(
    _In_ PVOID Object
    )
{
    PPH_LIST listArray = NULL;

    if (PhGetJsonObjectType(Object) == PH_JSON_OBJECT_TYPE_OBJECT)
    {
        json_object_iter iter;

        listArray = PhCreateList(1);

        json_object_object_foreachC(Object, iter)
        {
            PJSON_ARRAY_LIST_OBJECT object;

            object = PhAllocateZero(sizeof(JSON_ARRAY_LIST_OBJECT));
            object->Key = iter.key;
            object->Entry = iter.val;

            PhAddItemList(listArray, object);
        }
    }

    return listArray;
}

NTSTATUS PhLoadJsonObjectFromFile(
    _Out_ PVOID* JsonObject,
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    PPH_BYTES content;

    if (NT_SUCCESS(status = PhFileReadAllText(&content, FileName, FALSE)))
    {
        status = PhCreateJsonParserEx(JsonObject, content, FALSE);
        PhDereferenceObject(content);
    }

    return status;
}

static CONST PH_FLAG_MAPPING PhJsonFormatFlagMappings[] =
{
    { PH_JSON_TO_STRING_PLAIN, JSON_C_TO_STRING_PLAIN },
    { PH_JSON_TO_STRING_SPACED, JSON_C_TO_STRING_SPACED },
    { PH_JSON_TO_STRING_PRETTY, JSON_C_TO_STRING_PRETTY },
};

static VOID PhJsonObjectToJsonStringIndent(
    _In_ PPH_BYTES_BUILDER StringBuilder,
    _In_ ULONG level,
    _In_ ULONG flags
    )
{
    if (FlagOn(flags, JSON_C_TO_STRING_PRETTY))
    {
        // Print the indentation 'level' times
        for (ULONG i = 0; i < level; i++)
        {
            if (FlagOn(flags, JSON_C_TO_STRING_PRETTY_TAB))
            {
                PhAppendBytesBuilder2(StringBuilder, "\t");
            }
            else
            {
                // Standard 2-space indentation
                //PhAppendBytesBuilder2(StringBuilder, "  ");
            }
        }
    }
}

static void PhJsonObjectToJsonStringEscape(
    _In_ PPH_BYTES_BUILDER StringBuilder,
    _In_ PCSTR JsonString,
    _In_ SIZE_T Length,
    _In_ ULONG Flags
    )
{
    SIZE_T pos = 0;
    SIZE_T offset = 0;
    UCHAR c;

    while (Length)
    {
        --Length;
        c = JsonString[pos];

        switch (c)
        {
        case '\b':
        case '\n':
        case '\r':
        case '\t':
        case '\f':
        case '"':
        case '\\':
        case '/':
            {
                if ((Flags & JSON_C_TO_STRING_NOSLASHESCAPE) && c == '/')
                {
                    pos++;
                    break;
                }

                if (pos > offset)
                {
                    PH_BYTESREF string;
                    string.Buffer = (PCH)(JsonString + offset);
                    string.Length = pos - offset;
                    PhAppendBytesBuilder(StringBuilder, &string);
                }

                if (c == '\b') PhAppendBytesBuilder2(StringBuilder, "\\b");
                else if (c == '\n') PhAppendBytesBuilder2(StringBuilder, "\\n");
                else if (c == '\r') PhAppendBytesBuilder2(StringBuilder, "\\r");
                else if (c == '\t') PhAppendBytesBuilder2(StringBuilder, "\\t");
                else if (c == '\f') PhAppendBytesBuilder2(StringBuilder, "\\f");
                else if (c == '"') PhAppendBytesBuilder2(StringBuilder, "\\\"");
                else if (c == '\\') PhAppendBytesBuilder2(StringBuilder, "\\\\");
                else if (c == '/') PhAppendBytesBuilder2(StringBuilder, "\\/");

                offset = ++pos;
            }
            break;
        default:
            {
                if (c < ' ')
                {
                    if (pos > offset)
                    {
                        PH_BYTESREF string;
                        string.Buffer = (PCH)(JsonString + offset);
                        string.Length = pos - offset;
                        PhAppendBytesBuilder(StringBuilder, &string);
                    }

                    char buffer[7];
                    const char* json_hex_chars = "0123456789abcdefABCDEF";
                    sprintf_s(buffer, sizeof(buffer), "\\u00%c%c", json_hex_chars[c >> 4], json_hex_chars[c & 0xf]);
                    
                    PH_BYTESREF string;
                    string.Buffer = buffer;
                    string.Length = (SIZE_T)strlen(buffer);
                    PhAppendBytesBuilder(StringBuilder, &string);

                    offset = ++pos;
                }
                else
                {
                    pos++;
                }
            }
            break;
        }
    }

    if (pos > offset)
    {
        PH_BYTESREF string;
        string.Buffer = (PCH)(JsonString + offset);
        string.Length = pos - offset;
        PhAppendBytesBuilder(StringBuilder, &string);
    }
}

static BOOLEAN PhInternalJsonObjectObjectToString(
    _In_ PPH_BYTES_BUILDER StringBuilder,
    _In_ PVOID Object,
    _In_ ULONG Level,
    _In_ ULONG Flags
    )
{
    struct json_object_iterator it = json_object_iter_begin(Object);
    struct json_object_iterator itEnd = json_object_iter_end(Object);
    BOOLEAN had_children = FALSE;

    PhAppendBytesBuilder2(StringBuilder, "{");

    while (!json_object_iter_equal(&it, &itEnd)) 
    {
        const char* key = json_object_iter_peek_name(&it);
        struct json_object* val = json_object_iter_peek_value(&it);

        if (had_children)
        {
            PhAppendBytesBuilder2(StringBuilder, ",");
        }

        if (FlagOn(Flags, JSON_C_TO_STRING_PRETTY))
        {
            PhAppendBytesBuilder2(StringBuilder, "\n");
        }

        had_children = TRUE;

        if ((Flags & JSON_C_TO_STRING_SPACED) && !(Flags & JSON_C_TO_STRING_PRETTY))
        {
            PhAppendBytesBuilder2(StringBuilder, " ");
        }

        PhJsonObjectToJsonStringIndent(StringBuilder, Level + 1, Flags);

        PhAppendBytesBuilder2(StringBuilder, "\"");
        PhJsonObjectToJsonStringEscape(StringBuilder, key, strlen(key), Flags);
        PhAppendBytesBuilder2(StringBuilder, "\"");

        if (FlagOn(Flags, JSON_C_TO_STRING_SPACED))
            PhAppendBytesBuilder2(StringBuilder, ": ");
        else
            PhAppendBytesBuilder2(StringBuilder, ":");

        if (val == NULL)
        {
            PhAppendBytesBuilder2(StringBuilder, "null");
        }
        else if (!PhJsonObjectToString(StringBuilder, val, Level + 1, Flags))
        {
            return FALSE;
        }

        json_object_iter_next(&it);
    }

    if (FlagOn(Flags, JSON_C_TO_STRING_PRETTY) && had_children)
    {
        PhAppendBytesBuilder2(StringBuilder, "\n");
        PhJsonObjectToJsonStringIndent(StringBuilder, Level, Flags);
    }

    if (FlagOn(Flags, JSON_C_TO_STRING_SPACED) && !(Flags & JSON_C_TO_STRING_PRETTY))
    {
        PhAppendBytesBuilder2(StringBuilder, " }");
    }
    else
    {
        PhAppendBytesBuilder2(StringBuilder, "}");
    }

    return TRUE;
}

static BOOLEAN PhInternalJsonObjectArrayToString(
    _In_ PPH_BYTES_BUILDER StringBuilder,
    _In_ PVOID Object,
    _In_ ULONG Level,
    _In_ ULONG Flags
    )
{
    SIZE_T length = json_object_array_length(Object);
    SIZE_T i;

    PhAppendBytesBuilder2(StringBuilder, "[");

    for (i = 0; i < length; i++)
    {
        struct json_object* value = json_object_array_get_idx(Object, i);

        if (i > 0)
        {
            PhAppendBytesBuilder2(StringBuilder, ",");
        }

        if (FlagOn(Flags, JSON_C_TO_STRING_PRETTY))
        {
            PhAppendBytesBuilder2(StringBuilder, "\n");
        }

        if ((Flags & JSON_C_TO_STRING_SPACED) && !(Flags & JSON_C_TO_STRING_PRETTY))
        {
            PhAppendBytesBuilder2(StringBuilder, " ");
        }

        PhJsonObjectToJsonStringIndent(StringBuilder, Level + 1, Flags);

        if (value == NULL) // Should not happen in json-c arrays
        {
             PhAppendBytesBuilder2(StringBuilder, "null");
        }
        else
        {
            if (!PhJsonObjectToString(StringBuilder, value, Level + 1, Flags))
                return FALSE;
        }
    }

    if (FlagOn(Flags, JSON_C_TO_STRING_PRETTY) && length > 0)
    {
        PhAppendBytesBuilder2(StringBuilder, "\n");
        PhJsonObjectToJsonStringIndent(StringBuilder, Level, Flags);
    }

    if (FlagOn(Flags, JSON_C_TO_STRING_SPACED) && !(Flags & JSON_C_TO_STRING_PRETTY))
        PhAppendBytesBuilder2(StringBuilder, " ]");
    else
        PhAppendBytesBuilder2(StringBuilder, "]");

    return TRUE;
}

BOOLEAN PhJsonObjectToString(
    _In_ PPH_BYTES_BUILDER StringBuilder,
    _In_ PVOID Object,
    _In_ ULONG Level,
    _In_ ULONG Flags
    )
{
    switch (json_object_get_type(Object))
    {
    case json_type_null:
        {
            PhAppendBytesBuilder2(StringBuilder, "null");
        }
        break;
    case json_type_boolean:
        {
            if (json_object_get_boolean(Object))
                PhAppendBytesBuilder2(StringBuilder, "true");
            else
                PhAppendBytesBuilder2(StringBuilder, "false");
        }
        break;
    case json_type_double:
        {
            SIZE_T returnLength;
            CHAR formatBuffer[_CVTBUFSIZE + 1];

            if (NT_SUCCESS(PhFormatDoubleToUtf8(
                json_object_get_double(Object),
                FormatStandardForm,
                -1,
                formatBuffer,
                sizeof(formatBuffer),
                &returnLength
                )))
            {
                PH_BYTESREF stringFormat;

                stringFormat.Buffer = formatBuffer;
                stringFormat.Length = returnLength - sizeof(ANSI_NULL);

                PhAppendBytesBuilder(StringBuilder, &stringFormat);
            }
            else
            {
                CHAR buffer[64];

                // %.17g is standard to preserve double precision in text
                if (sprintf_s(buffer, sizeof(buffer), "%.17g", json_object_get_double(Object)) > 0)
                {
                    // Handle specific JSON quirk: if double looks like int (no dot), append .0?
                    // Standard JSON-C usually handles this, but strictly speaking "1" is a valid double in JSON.
                    // We stick to the string output.
                    PhAppendBytesBuilder2(StringBuilder, buffer);
                }
            }
        }
        break;
    case json_type_int:
        {
            SIZE_T returnLength;
            //PH_FORMAT format[1];
            //WCHAR formatBuffer[260];
            //
            //PhInitFormatI64D(&format[0], json_object_get_int64(Object));
            //
            //if (PhFormatToBuffer(
            //    format,
            //    RTL_NUMBER_OF(format),
            //    formatBuffer,
            //    sizeof(formatBuffer),
            //    &returnLength
            //    ))
            //{
            //    PH_BYTESREF stringFormat;
            //
            //    stringFormat.Buffer = formatBuffer;
            //    stringFormat.Length = returnLength - sizeof(ANSI_NULL);
            //
            //    PhAppendBytesBuilder(StringBuilder, &stringFormat);
            //}
            //else
            {
                CHAR formatBuffer[65];

                if (NT_SUCCESS(PhIntegerToUtf8Buffer(
                    json_object_get_int64(Object),
                    10,
                    TRUE,
                    formatBuffer,
                    sizeof(formatBuffer),
                    &returnLength
                    )))
                {
                    PH_BYTESREF stringFormat;

                    stringFormat.Buffer = formatBuffer;
                    stringFormat.Length = returnLength - sizeof(ANSI_NULL);

                    PhAppendBytesBuilder(StringBuilder, &stringFormat);
                }
                else
                {
                    if (sprintf_s(formatBuffer, sizeof(formatBuffer), "%lld", json_object_get_int64(Object)) > 0)
                    {
                        PhAppendBytesBuilder2(StringBuilder, formatBuffer);
                    }
                }
            }
        }
        break;
    case json_type_object:
        {
            return PhInternalJsonObjectObjectToString(StringBuilder, Object, Level, Flags);
        }
        break;
    case json_type_array:
        {
            return PhInternalJsonObjectArrayToString(StringBuilder, Object, Level, Flags);
        }
        break;
    case json_type_string:
        {
            PhAppendBytesBuilder2(StringBuilder, "\"");
            PhJsonObjectToJsonStringEscape(
                StringBuilder,
                json_object_get_string(Object),
                (size_t)json_object_get_string_len(Object),
                Flags
                );
            PhAppendBytesBuilder2(StringBuilder, "\"");
        }
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

PPH_BYTES PhJsonObjectToJsonString(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PH_BYTES_BUILDER stringBuilder;

    PhInitializeBytesBuilder(&stringBuilder, 25 * 1000);

    if (PhJsonObjectToString(&stringBuilder, Object, 0, Flags))
    {
        return PhFinalBytesBuilderBytes(&stringBuilder);
    }

    PhDeleteBytesBuilder(&stringBuilder);
    return NULL;
}

NTSTATUS PhSaveJsonObjectToFile(
    _In_ PCPH_STRINGREF FileName,
    _In_ PVOID Object,
    _In_opt_ ULONG Flags
    )
{
    static CONST PH_STRINGREF extension = PH_STRINGREF_INIT(L".backup");
    NTSTATUS status;
    ULONG json_flags = 0;
    HANDLE fileHandle = NULL;
    PPH_STRING fileName;
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER allocationSize;
    PPH_BYTES jsonStringUtf8;
    //SIZE_T json_length;
    //PCSTR json_string;

    json_flags = 0;
    PhMapFlags1(&json_flags, Flags, PhJsonFormatFlagMappings, RTL_NUMBER_OF(PhJsonFormatFlagMappings));

    jsonStringUtf8 = PhJsonObjectToJsonString(Object, json_flags);

    //json_string = json_object_to_json_string_length(
    //    Object,
    //    json_flags,
    //    &json_length
    //    );

    if (!jsonStringUtf8 || jsonStringUtf8->Length == 0)
        return STATUS_UNSUCCESSFUL;

    // Preallocate the file size.

    allocationSize.QuadPart = jsonStringUtf8->Length;

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
        (PVOID)jsonStringUtf8->Buffer,
        (ULONG)jsonStringUtf8->Length,
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

    if (jsonStringUtf8)
    {
        PhDereferenceObject(jsonStringUtf8);
    }

    return status;
}

// XML support

static mxml_node_t* PhXmlLoadString(
    _In_ PCSTR String
    )
{
    mxml_options_t* options;
    mxml_node_t* currentNode;

    options = mxmlOptionsNew();
    mxmlOptionsSetTypeValue(options, MXML_TYPE_OPAQUE);

    currentNode = mxmlLoadString(NULL, options, String);

    mxmlOptionsDelete(options);

    return currentNode;
}

static PPH_BYTES PhXmlSaveString(
    _In_ PVOID XmlRootObject,
    _In_opt_ PVOID XmlSaveCallback
    )
{
    mxml_options_t* options;
    PPH_BYTES string;

    options = mxmlOptionsNew();
    mxmlOptionsSetTypeValue(options, MXML_TYPE_OPAQUE);

    if (XmlSaveCallback)
    {
        mxmlOptionsSetWhitespaceCallback(options, XmlSaveCallback, NULL);
    }

    string = PhCreateBytes(mxmlSaveAllocString(XmlRootObject, options));
    mxmlOptionsDelete(options);

    return string;
}

PVOID PhLoadXmlObjectFromString(
    _In_ PCSTR String
    )
{
    mxml_node_t* currentNode;

    if (currentNode = PhXmlLoadString(String))
    {
        if (mxmlGetType(currentNode) == MXML_TYPE_ELEMENT)
        {
            return currentNode;
        }

        mxmlDelete(currentNode);
    }

    return NULL;
}

PVOID PhLoadXmlObjectFromString2(
    _In_ PCSTR String
    )
{
    mxml_node_t* currentNode;

    if (currentNode = PhXmlLoadString(String))
    {
        return currentNode;
    }

    return NULL;
}

NTSTATUS PhLoadXmlObjectFromFile(
    _In_ PCPH_STRINGREF FileName,
    _Out_opt_ PVOID* XmlRootObject
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    PPH_BYTES fileContent;
    mxml_node_t* currentNode = NULL;

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

    if (NT_SUCCESS(status = PhGetFileText(&fileContent, fileHandle, FALSE)))
    {
        currentNode = PhXmlLoadString(fileContent->Buffer);
        PhDereferenceObject(fileContent);
    }

    NtClose(fileHandle);

    if (currentNode)
    {
        if (mxmlGetType(currentNode) == MXML_TYPE_ELEMENT)
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
    _In_ PCPH_STRINGREF FileName,
    _In_ PVOID XmlRootObject,
    _In_opt_ PVOID XmlSaveCallback
    )
{
    static CONST PH_STRINGREF extension = PH_STRINGREF_INIT(L".tmp");
    NTSTATUS status;
    PPH_STRING fileName;
    HANDLE fileHandle = NULL;
    LARGE_INTEGER allocationSize;
    PPH_BYTES string;

    string = PhXmlSaveString(XmlRootObject, XmlSaveCallback);

    if (!string)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    allocationSize.QuadPart = string->Length;

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

    status = PhWriteFile(
        fileHandle,
        (PVOID)string->Buffer,
        (ULONG)string->Length,
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

    if (string)
    {
        PhDereferenceObject(string);
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
    return mxmlFindElement(XmlNodeObject, XmlTopObject, Element, Attribute, Value, MXML_DESCEND_ALL);
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
    _In_ SIZE_T Index,
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

SIZE_T PhGetXmlNodeAttributeCount(
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

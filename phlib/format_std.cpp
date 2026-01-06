/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2024
 *
 */

#include <phbase.h>
#include <charconv>

extern NTSTATUS PhErrcToNtStatus(
    _In_ std::errc ec
    );

NTSTATUS PhFormatSingleToUtf8(
    _In_ FLOAT Value,
    _In_ ULONG Type,
    _In_ LONG Precision,
    _Out_writes_bytes_(BufferLength) PSTR Buffer,
    _In_opt_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
#if defined(PH_TOCHARS_BUFFER)
    chars_format format = chars_format::fixed;
    to_chars_result result;
    SIZE_T returnLength;
    CHAR buffer[_CVTBUFSIZE + 1];

    if (Type & FormatStandardForm)
        format = chars_format::general;
    else if (Type & FormatHexadecimalForm)
        format = chars_format::hex;

    result = std::to_chars(
        buffer,
        Buffer + BufferLength - sizeof(ANSI_NULL), // Reserve space for null terminator // end(buffer)
        Value,
        format,
        Precision
        );

    if (result.ec != static_cast<std::errc>(0))
        return PhErrcToNtStatus(result.ec);

    returnLength = result.ptr - buffer;

    if (returnLength == 0)
        return STATUS_UNSUCCESSFUL;

    // This could be removed in favor of directly passing the input buffer to std:to_chars but
    // for now use memcpy so that failures writing a value don't touch the input buffer (dmex)
    memcpy_s(Buffer, BufferLength, buffer, returnLength);
    Buffer[returnLength] = ANSI_NULL;

    if (ReturnLength)
        *ReturnLength = returnLength;

    return STATUS_SUCCESS;
#else
    std::chars_format format;
    std::to_chars_result result;
    SIZE_T returnLength;

    if (Precision < 0)
    {
        // This overload finds the shortest string that converts back to the exact same double.
        // It is roughly 2x faster than the version with explicit precision.
        result = std::to_chars(
            Buffer,
            Buffer + BufferLength - sizeof(ANSI_NULL),
            Value
            );
    }
    else
    {
        if (FlagOn(Type, FormatStandardForm))
            format = std::chars_format::general;
        else if (FlagOn(Type, FormatHexadecimalForm))
            format = std::chars_format::hex;
        else
            format = std::chars_format::fixed;

        result = std::to_chars(
            Buffer,
            Buffer + BufferLength - sizeof(ANSI_NULL), // Reserve space for null terminator // end(buffer)
            Value,
            format,
            Precision
            );
    }

    if (result.ec != static_cast<std::errc>(0))
        return PhErrcToNtStatus(result.ec);

    returnLength = result.ptr - Buffer;

    if (ReturnLength)
    {
        *ReturnLength = returnLength;
    }

    Buffer[returnLength] = ANSI_NULL;

    return STATUS_SUCCESS;
#endif
}

NTSTATUS PhFormatDoubleToUtf8(
    _In_ DOUBLE Value,
    _In_ ULONG Type,
    _In_ LONG Precision,
    _Out_writes_bytes_(BufferLength) PSTR Buffer,
    _In_opt_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
#if defined(PH_TOCHARS_BUFFER)
    chars_format format = chars_format::fixed;
    to_chars_result result;
    SIZE_T returnLength;
    CHAR buffer[_CVTBUFSIZE + 1];

    if (Precision < 0)
    {
        // This overload finds the shortest string that converts back to the exact same double.
        // It is roughly 2x faster than the version with explicit precision.
        result = std::to_chars(
            Buffer,
            Buffer + BufferLength - sizeof(ANSI_NULL),
            Value
            );
    }
    else
    {
        if (FlagOn(Type, FormatStandardForm))
            format = std::chars_format::general;
        else if (FlagOn(Type, FormatHexadecimalForm))
            format = std::chars_format::hex;
        else
            format = std::chars_format::fixed;

        result = to_chars(
            buffer,
            Buffer + BufferLength - sizeof(ANSI_NULL), // end(buffer)
            Value,
            format,
            Precision
            );
    }

    if (result.ec != static_cast<std::errc>(0))
        return STATUS_UNSUCCESSFUL;

    returnLength = result.ptr - buffer;

    if (returnLength == 0)
        return STATUS_UNSUCCESSFUL;

    // This could be removed in favor of directly passing the input buffer to std:to_chars but
    // for now use memcpy so that failures writing a value don't touch the input buffer (dmex)
    memcpy_s(Buffer, BufferLength, buffer, returnLength);
    Buffer[returnLength] = ANSI_NULL;

    if (ReturnLength)
        *ReturnLength = returnLength;

    return STATUS_SUCCESS;
#else
    std::chars_format format;
    std::to_chars_result result;
    SIZE_T returnLength;

    if (Precision < 0)
    {
        // This overload finds the shortest string that converts back to the exact same double.
        // It is roughly 2x faster than the version with explicit precision.
        result = std::to_chars(
            Buffer,
            Buffer + BufferLength - sizeof(ANSI_NULL),
            Value
            );
    }
    else
    {
        if (FlagOn(Type, FormatStandardForm))
            format = std::chars_format::general;
        else if (FlagOn(Type, FormatHexadecimalForm))
            format = std::chars_format::hex;
        else
            format = std::chars_format::fixed;

        result = std::to_chars(
            Buffer,
            Buffer + BufferLength - sizeof(ANSI_NULL), // Reserve space for null terminator
            Value,
            format,
            Precision
            );
    }

    if (result.ec != static_cast<std::errc>(0))
        return PhErrcToNtStatus(result.ec);

    returnLength = result.ptr - Buffer;

    if (ReturnLength)
    {
        *ReturnLength = returnLength;
    }

    Buffer[returnLength] = ANSI_NULL;

    return STATUS_SUCCESS;
#endif
}

/**
 * Converts an integer to a UTF-8 string buffer (Replicates PhIntegerToString64).
 * 
 * \param Integer The integer to convert.
 * \param Base The radix (e.g., 10, 16, 8). If 0, defaults to 10.
 * \param Signed TRUE to treat as signed int64, FALSE for unsigned int64.
 * \param Buffer The output buffer.
 * \param BufferLength The size of the buffer in bytes.
 * \param ReturnLength Contains the number of bytes written (excluding null terminator).
 */
NTSTATUS PhIntegerToUtf8Buffer(
    _In_ LONG64 Integer,
    _In_opt_ ULONG Base,
    _In_ BOOLEAN Signed,
    _Out_writes_bytes_(BufferLength) PSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    std::to_chars_result result;
    INT radix = (Base == 0) ? 10 : (INT)Base;

    if (!Buffer || BufferLength == 0)
        return STATUS_BUFFER_TOO_SMALL;
    if (radix < 2 || radix > 36)
        return STATUS_INVALID_PARAMETER;

    // Reserve 1 byte for null terminator
    PSTR endBuffer = Buffer + BufferLength - sizeof(ANSI_NULL);

    if (Signed)
    {
        result = std::to_chars(
            Buffer, 
            endBuffer, 
            (long long)Integer, 
            radix
            );
    }
    else
    {
        result = std::to_chars(
            Buffer, 
            endBuffer, 
            (unsigned long long)Integer, 
            radix
            );
    }

    if (result.ec != std::errc())
    {
        if (result.ec == std::errc::value_too_large)
            return STATUS_BUFFER_OVERFLOW;
        
        return STATUS_UNSUCCESSFUL;
    }

    *result.ptr = ANSI_NULL; //Null-terminate

    if (ReturnLength)
    {
        SIZE_T length = result.ptr - Buffer; // Calculate written length

        *ReturnLength = length;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhErrcToNtStatus(
    _In_ std::errc ec
    )
{
    switch (ec)
    {
    case std::errc::address_family_not_supported:
        return STATUS_NOT_SUPPORTED;
    case std::errc::address_in_use:
        return STATUS_ADDRESS_ALREADY_ASSOCIATED;
    case std::errc::address_not_available:
        return STATUS_ADDRESS_NOT_ASSOCIATED;
    case std::errc::already_connected:
        return STATUS_CONNECTION_ACTIVE;
    case std::errc::argument_list_too_long:
    case std::errc::argument_out_of_domain:
        return STATUS_INVALID_PARAMETER;
    case std::errc::bad_address:
        return STATUS_INVALID_ADDRESS;
    case std::errc::bad_file_descriptor:
        return STATUS_INVALID_HANDLE;
    case std::errc::bad_message:
        return STATUS_DATA_ERROR;
    case std::errc::broken_pipe:
        return STATUS_PIPE_BROKEN;
    case std::errc::connection_aborted:
        return STATUS_CONNECTION_ABORTED;
    case std::errc::connection_already_in_progress:
        return STATUS_CONNECTION_ACTIVE;
    case std::errc::connection_refused:
        return STATUS_CONNECTION_REFUSED;
    case std::errc::connection_reset:
        return STATUS_CONNECTION_RESET;
    case std::errc::cross_device_link:
        return STATUS_NOT_SAME_DEVICE;
    case std::errc::destination_address_required:
        return STATUS_DESTINATION_ELEMENT_FULL;
    case std::errc::device_or_resource_busy:
        return STATUS_DEVICE_BUSY;
    case std::errc::directory_not_empty:
        return STATUS_DIRECTORY_NOT_EMPTY;
    case std::errc::executable_format_error:
        return STATUS_INVALID_IMAGE_FORMAT;
    case std::errc::file_exists:
        return STATUS_OBJECT_NAME_COLLISION;
    case std::errc::file_too_large:
        return STATUS_FILE_TOO_LARGE;
    case std::errc::filename_too_long:
        return STATUS_NAME_TOO_LONG;
    case std::errc::function_not_supported:
        return STATUS_NOT_SUPPORTED;
    case std::errc::host_unreachable:
        return STATUS_HOST_UNREACHABLE;
    case std::errc::identifier_removed:
        return STATUS_OBJECT_NAME_NOT_FOUND;
    case std::errc::illegal_byte_sequence:
        return STATUS_ILLEGAL_CHARACTER;
    case std::errc::inappropriate_io_control_operation:
        return STATUS_INVALID_DEVICE_REQUEST;
    case std::errc::interrupted:
        return STATUS_ALERTED;
    case std::errc::invalid_argument:
    case std::errc::invalid_seek:
        return STATUS_INVALID_PARAMETER;
    case std::errc::io_error:
        return STATUS_IO_DEVICE_ERROR;
    case std::errc::is_a_directory:
        return STATUS_FILE_IS_A_DIRECTORY;
    case std::errc::message_size:
        return STATUS_BUFFER_OVERFLOW;
    case std::errc::network_down:
        return STATUS_NETWORK_UNREACHABLE;
    case std::errc::network_reset:
        return STATUS_CONNECTION_RESET;
    case std::errc::network_unreachable:
        return STATUS_NETWORK_UNREACHABLE;
    case std::errc::no_buffer_space:
        return STATUS_INSUFFICIENT_RESOURCES;
    case std::errc::no_child_process:
        return STATUS_INVALID_CID;
    case std::errc::no_link:
        return STATUS_NOT_FOUND;
    case std::errc::no_lock_available:
        return STATUS_LOCK_NOT_GRANTED;
    case std::errc::no_message:
        return STATUS_NO_DATA_DETECTED;
    case std::errc::no_protocol_option:
        return STATUS_PROTOCOL_UNREACHABLE;
    case std::errc::no_space_on_device:
        return STATUS_DISK_FULL;
    case std::errc::no_such_device_or_address:
        return STATUS_INVALID_ADDRESS;
    case std::errc::no_such_device:
        return STATUS_NO_SUCH_DEVICE;
    case std::errc::no_such_file_or_directory:
        return STATUS_OBJECT_NAME_NOT_FOUND;
    case std::errc::no_such_process:
        return STATUS_INVALID_CID;
    case std::errc::not_a_directory:
        return STATUS_NOT_A_DIRECTORY;
    case std::errc::not_a_socket:
        return STATUS_OBJECT_TYPE_MISMATCH;
    case std::errc::not_connected:
        return STATUS_CONNECTION_DISCONNECTED;
    case std::errc::not_enough_memory:
        return STATUS_NO_MEMORY;
    case std::errc::not_supported:
        return STATUS_NOT_SUPPORTED;
    case std::errc::operation_canceled:
        return STATUS_CANCELLED;
    case std::errc::operation_in_progress:
        return STATUS_PENDING;
    case std::errc::operation_not_permitted:
        return STATUS_PRIVILEGE_NOT_HELD;
    case std::errc::operation_not_supported:
        return STATUS_NOT_SUPPORTED;
    case std::errc::operation_would_block:
        return STATUS_DEVICE_BUSY;
    case std::errc::owner_dead:
        return STATUS_INVALID_CID;
    case std::errc::permission_denied:
        return STATUS_ACCESS_DENIED;
    case std::errc::protocol_error:
    case std::errc::protocol_not_supported:
        return STATUS_PROTOCOL_UNREACHABLE;
    case std::errc::read_only_file_system:
        return STATUS_MEDIA_WRITE_PROTECTED;
    case std::errc::resource_deadlock_would_occur:
        return STATUS_POSSIBLE_DEADLOCK;
    case std::errc::resource_unavailable_try_again:
        return STATUS_DEVICE_BUSY;
    case std::errc::result_out_of_range:
        return STATUS_INTEGER_OVERFLOW;
    case std::errc::state_not_recoverable:
        return STATUS_UNSUCCESSFUL;
    case std::errc::text_file_busy:
        return STATUS_FILE_LOCK_CONFLICT;
    case std::errc::timed_out:
        return STATUS_TIMEOUT;
    case std::errc::too_many_files_open_in_system:
    case std::errc::too_many_files_open:
        return STATUS_TOO_MANY_OPENED_FILES;
    case std::errc::too_many_links:
    case std::errc::too_many_symbolic_link_levels:
        return STATUS_TOO_MANY_LINKS;
    case std::errc::value_too_large:
        return STATUS_INTEGER_OVERFLOW;
    case std::errc::wrong_protocol_type:
        return STATUS_PROTOCOL_UNREACHABLE;
    case std::errc():
        return STATUS_SUCCESS;
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

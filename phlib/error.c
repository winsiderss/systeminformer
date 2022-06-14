/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *
 */

#include <phbase.h>

/**
 * Converts a NTSTATUS value to a Win32 error code.
 *
 * \remarks This function handles FACILITY_NTWIN32 status values properly, unlike
 * RtlNtStatusToDosError.
 */
ULONG PhNtStatusToDosError(
    _In_ NTSTATUS Status
    )
{
    if (NT_NTWIN32(Status)) // RtlNtStatusToDosError doesn't seem to handle these cases correctly
        return WIN32_FROM_NTSTATUS(Status);
    else
        return RtlNtStatusToDosErrorNoTeb(Status);
}

/**
 * Converts a Win32 error code to a NTSTATUS value.
 *
 * \remarks Only a small number of cases are currently supported. Other status values are wrapped
 * using FACILITY_NTWIN32.
 */
NTSTATUS PhDosErrorToNtStatus(
    _In_ ULONG DosError
    )
{
    switch (DosError)
    {
    case ERROR_INVALID_FUNCTION: return STATUS_ILLEGAL_FUNCTION;
    case ERROR_FILE_NOT_FOUND: return STATUS_NO_SUCH_FILE;
    case ERROR_ACCESS_DENIED: return STATUS_ACCESS_DENIED;
    case ERROR_INVALID_HANDLE: return STATUS_INVALID_HANDLE;
    case ERROR_HANDLE_EOF: return STATUS_END_OF_FILE;
    case ERROR_NOT_SUPPORTED: return STATUS_NOT_SUPPORTED;
    case ERROR_INVALID_PARAMETER: return STATUS_INVALID_PARAMETER;
    case ERROR_NOT_LOCKED: return STATUS_NOT_LOCKED;
    case ERROR_MORE_DATA: return STATUS_MORE_ENTRIES;
    case ERROR_NOACCESS: return STATUS_ACCESS_VIOLATION;
    case ERROR_STACK_OVERFLOW: return STATUS_STACK_OVERFLOW;
    case ERROR_INTERNAL_ERROR: return STATUS_INTERNAL_ERROR;
    default: return NTSTATUS_FROM_WIN32(DosError);
    }
}

/**
 * Determines whether a NTSTATUS value indicates that a file cannot be not found.
 */
BOOLEAN PhNtStatusFileNotFound(
    _In_ NTSTATUS Status
    )
{
    switch (Status)
    {
    case STATUS_NO_SUCH_FILE:
        return TRUE;
    case STATUS_OBJECT_NAME_INVALID:
        return TRUE;
    case STATUS_OBJECT_NAME_NOT_FOUND:
        return TRUE;
    case STATUS_OBJECT_NO_LONGER_EXISTS:
        return TRUE;
    case STATUS_OBJECT_PATH_INVALID:
        return TRUE;
    case STATUS_OBJECT_PATH_NOT_FOUND:
        return TRUE;
    default: return FALSE;
    }
}

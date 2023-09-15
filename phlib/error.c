/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2018-2023
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
 * Converts a NTSTATUS value to a Win32 service error code.
 */
ULONG PhNtStatusToServiceStatus(
    _In_ NTSTATUS Status
    )
{
    switch (Status)
    {
    case STATUS_SUCCESS: return ERROR_SUCCESS;
    case STATUS_SERVICE_NOTIFICATION: return ERROR_SERVICE_NOTIFICATION;
    case STATUS_UNSATISFIED_DEPENDENCIES: return ERROR_DEPENDENT_SERVICES_RUNNING;
    case STATUS_IMAGE_ALREADY_LOADED: return ERROR_SERVICE_ALREADY_RUNNING;
    case STATUS_AUDITING_DISABLED: return ERROR_SERVICE_DISABLED; // ??
    case STATUS_OBJECT_NAME_NOT_FOUND: return ERROR_SERVICE_DOES_NOT_EXIST;
    case STATUS_OBJECT_NAME_COLLISION: return ERROR_SERVICE_EXISTS;
    case STATUS_OBJECT_NAME_EXISTS: return ERROR_DUPLICATE_SERVICE_NAME;
    case STATUS_OBJECT_PATH_INVALID: return ERROR_SERVICE_NOT_FOUND;
    case STATUS_SERVICES_FAILED_AUTOSTART: return ERROR_SERVICES_FAILED_AUTOSTART;
    default: { return ERROR_MR_MID_NOT_FOUND; }
    }
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
    if (NT_CUSTOMER(DosError))
        return DosError;

    switch (DosError)
    {
    case ERROR_SUCCESS: return STATUS_SUCCESS;
    case ERROR_INVALID_FUNCTION: return STATUS_ILLEGAL_FUNCTION;
    case ERROR_FILE_NOT_FOUND: return STATUS_NO_SUCH_FILE;
    case ERROR_ACCESS_DENIED: return STATUS_ACCESS_DENIED;
    case ERROR_INVALID_HANDLE: return STATUS_INVALID_HANDLE;
    case ERROR_NO_MORE_FILES: return STATUS_NO_MORE_FILES;
    case ERROR_SHARING_VIOLATION: return STATUS_SHARING_VIOLATION;
    case ERROR_HANDLE_EOF: return STATUS_END_OF_FILE;
    case ERROR_NOT_SUPPORTED: return STATUS_NOT_SUPPORTED;
    case ERROR_INVALID_PARAMETER: return STATUS_INVALID_PARAMETER;
    case ERROR_INSUFFICIENT_BUFFER: return STATUS_BUFFER_TOO_SMALL;
    case ERROR_MOD_NOT_FOUND: return STATUS_DLL_NOT_FOUND;
    case ERROR_PROC_NOT_FOUND: return STATUS_PROCEDURE_NOT_FOUND;
    case ERROR_NOT_LOCKED: return STATUS_NOT_LOCKED;
    case ERROR_MORE_DATA: return STATUS_MORE_ENTRIES;
    case ERROR_NO_MORE_ITEMS: return STATUS_NO_MORE_ENTRIES;
    case ERROR_PARTIAL_COPY: return STATUS_PARTIAL_COPY;
    case ERROR_NOINTERFACE: return STATUS_NOINTERFACE;
    case ERROR_SERVICE_NOTIFICATION: return STATUS_SERVICE_NOTIFICATION;
    case ERROR_ALERTED: return STATUS_ALERTED;
    case ERROR_ELEVATION_REQUIRED: return STATUS_ELEVATION_REQUIRED;
    case ERROR_NOACCESS: return STATUS_ACCESS_VIOLATION;
    case ERROR_STACK_OVERFLOW: return STATUS_STACK_OVERFLOW;
    case ERROR_DEPENDENT_SERVICES_RUNNING: return STATUS_UNSATISFIED_DEPENDENCIES;
    case ERROR_SERVICE_ALREADY_RUNNING: return STATUS_IMAGE_ALREADY_LOADED;
    case ERROR_SERVICE_DISABLED: return STATUS_AUDITING_DISABLED; // ??
    case ERROR_SERVICE_DOES_NOT_EXIST: return STATUS_OBJECT_NAME_NOT_FOUND;
    case ERROR_SERVICE_EXISTS: return STATUS_OBJECT_NAME_COLLISION;
    case ERROR_DUPLICATE_SERVICE_NAME: return STATUS_OBJECT_NAME_EXISTS;
    case ERROR_SERVICE_NOT_FOUND: return STATUS_OBJECT_PATH_INVALID;
    case ERROR_SOME_NOT_MAPPED: return STATUS_SOME_NOT_MAPPED;
    case ERROR_PRIVILEGE_NOT_HELD: return STATUS_PRIVILEGE_NOT_HELD;
    case ERROR_NONE_MAPPED: return STATUS_NONE_MAPPED;
    case ERROR_INTERNAL_ERROR: return STATUS_INTERNAL_ERROR;
    case ERROR_SERVICES_FAILED_AUTOSTART: return STATUS_SERVICES_FAILED_AUTOSTART;
    default:
        {
            assert(FALSE); // Update the table. (dmex)
            return NTSTATUS_FROM_WIN32(DosError);
        }
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

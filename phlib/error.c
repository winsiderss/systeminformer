/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2018-2026
 *
 */

#include <phbase.h>

#ifndef PH_NATIVE_NTSTATUS_TO_DOS_ERROR
#define PH_NATIVE_NTSTATUS_TO_DOS_ERROR 1
#endif

typedef struct _PHP_DOS_ERROR_STATUS_ENTRY
{
    ULONG DosError;
    NTSTATUS Status;
} PHP_DOS_ERROR_STATUS_ENTRY;

static const PHP_DOS_ERROR_STATUS_ENTRY PhpDosErrorStatusTable[] =
{
    { ERROR_SUCCESS,                        STATUS_SUCCESS },
    { ERROR_INVALID_FUNCTION,               STATUS_ILLEGAL_FUNCTION },
    { ERROR_FILE_NOT_FOUND,                 STATUS_NO_SUCH_FILE },
    { ERROR_PATH_NOT_FOUND,                 STATUS_OBJECT_PATH_NOT_FOUND },
    { ERROR_ACCESS_DENIED,                  STATUS_ACCESS_DENIED },
    { ERROR_INVALID_HANDLE,                 STATUS_INVALID_HANDLE },
    { ERROR_NOT_ENOUGH_MEMORY,              STATUS_NO_MEMORY },
    { ERROR_INVALID_DATA,                   STATUS_DATA_ERROR },
    { ERROR_NO_MORE_FILES,                  STATUS_NO_MORE_FILES },
    { ERROR_BAD_LENGTH,                     STATUS_INFO_LENGTH_MISMATCH },
    { ERROR_SHARING_VIOLATION,              STATUS_SHARING_VIOLATION },
    { ERROR_HANDLE_EOF,                     STATUS_END_OF_FILE },
    { ERROR_NOT_SUPPORTED,                  STATUS_NOT_SUPPORTED },
    { ERROR_INVALID_PARAMETER,              STATUS_INVALID_PARAMETER },
    { ERROR_INSUFFICIENT_BUFFER,            STATUS_BUFFER_TOO_SMALL },
    { ERROR_INVALID_NAME,                   STATUS_OBJECT_NAME_INVALID },
    { ERROR_MOD_NOT_FOUND,                  STATUS_DLL_NOT_FOUND },
    { ERROR_PROC_NOT_FOUND,                 STATUS_PROCEDURE_NOT_FOUND },
    { ERROR_NOT_LOCKED,                     STATUS_NOT_LOCKED },
    { ERROR_BAD_PATHNAME,                   STATUS_OBJECT_PATH_INVALID },
    { ERROR_INVALID_ORDINAL,                STATUS_ORDINAL_NOT_FOUND },
    { ERROR_ALREADY_EXISTS,                 STATUS_OBJECT_NAME_COLLISION },
    { ERROR_BAD_EXE_FORMAT,                 STATUS_INVALID_IMAGE_FORMAT },
    { ERROR_DELETE_PENDING,                 STATUS_DELETE_PENDING },
    { ERROR_MORE_DATA,                      STATUS_MORE_ENTRIES },
    { ERROR_NO_MORE_ITEMS,                  STATUS_NO_MORE_ENTRIES },
    { ERROR_DIRECTORY,                      STATUS_OBJECT_NAME_INVALID },
    { ERROR_PARTIAL_COPY,                   STATUS_PARTIAL_COPY },
    { ERROR_INVALID_IMAGE_HASH,             STATUS_INVALID_IMAGE_HASH },
    { ERROR_NOINTERFACE,                    STATUS_NOINTERFACE },
    { ERROR_SERVICE_NOTIFICATION,           STATUS_SERVICE_NOTIFICATION },
    { ERROR_ALERTED,                        STATUS_ALERTED },
    { ERROR_ELEVATION_REQUIRED,             STATUS_ELEVATION_REQUIRED },
    { ERROR_NOACCESS,                       STATUS_ACCESS_VIOLATION },
    { ERROR_STACK_OVERFLOW,                 STATUS_STACK_OVERFLOW },
    { ERROR_NO_TOKEN,                       STATUS_NO_TOKEN },
    { ERROR_DEPENDENT_SERVICES_RUNNING,     STATUS_UNSATISFIED_DEPENDENCIES },
    { ERROR_SERVICE_REQUEST_TIMEOUT,        STATUS_TIMEOUT },
    { ERROR_SERVICE_ALREADY_RUNNING,        STATUS_IMAGE_ALREADY_LOADED },
    { ERROR_SERVICE_DISABLED,               STATUS_ILL_FORMED_SERVICE_ENTRY },
    { ERROR_SERVICE_DOES_NOT_EXIST,         STATUS_OBJECT_NAME_NOT_FOUND },
    { ERROR_SERVICE_EXISTS,                 STATUS_OBJECT_NAME_COLLISION },
    { ERROR_SERVICE_NEVER_STARTED,          STATUS_THREAD_NOT_RUNNING },
    { ERROR_PROCESS_ABORTED,                STATUS_FATAL_APP_EXIT },
    { ERROR_DUPLICATE_SERVICE_NAME,         STATUS_OBJECT_NAME_EXISTS },
    { ERROR_DLL_INIT_FAILED,                STATUS_DLL_INIT_FAILED },
    { ERROR_NOT_FOUND,                      STATUS_NOT_FOUND },
    { ERROR_CANCELLED,                      STATUS_CANCELLED },
    { ERROR_SERVICE_NOT_FOUND,              STATUS_OBJECT_PATH_INVALID },
    { ERROR_SOME_NOT_MAPPED,                STATUS_SOME_NOT_MAPPED },
    { ERROR_PRIVILEGE_NOT_HELD,             STATUS_PRIVILEGE_NOT_HELD },
    { ERROR_LOGON_FAILURE,                  STATUS_LOGON_FAILURE },
    { ERROR_ACCOUNT_RESTRICTION,            STATUS_ACCOUNT_RESTRICTION },
    { ERROR_ACCOUNT_DISABLED,               STATUS_ACCOUNT_DISABLED },
    { ERROR_NONE_MAPPED,                    STATUS_NONE_MAPPED },
    { ERROR_SERVER_DISABLED,                STATUS_SERVER_DISABLED },
    { ERROR_NO_SUCH_DOMAIN,                 STATUS_NO_SUCH_DOMAIN },
    { ERROR_INTERNAL_ERROR,                 STATUS_INTERNAL_ERROR },
    { ERROR_INVALID_WINDOW_HANDLE,          STATUS_INVALID_HANDLE },
    { ERROR_NO_SYSTEM_RESOURCES,            STATUS_INSUFFICIENT_RESOURCES },
    { ERROR_TIMEOUT,                        STATUS_TIMEOUT },
    { ERROR_INVALID_MONITOR_HANDLE,         STATUS_INVALID_HANDLE },
    { ERROR_DYNAMIC_CODE_BLOCKED,           STATUS_DYNAMIC_CODE_BLOCKED },
    { ERROR_STRICT_CFG_VIOLATION,           STATUS_STRICT_CFG_VIOLATION },
    { ERROR_RESOURCE_TYPE_NOT_FOUND,        STATUS_RESOURCE_TYPE_NOT_FOUND },
    { ERROR_RESOURCE_NAME_NOT_FOUND,        STATUS_RESOURCE_NAME_NOT_FOUND },
    { ERROR_RESOURCE_LANG_NOT_FOUND,        STATUS_RESOURCE_LANG_NOT_FOUND },
    { ERROR_NOT_ENOUGH_QUOTA,               STATUS_QUOTA_EXCEEDED },
    { ERROR_INVALID_TIME,                   STATUS_INVALID_PARAMETER },
    { ERROR_WMI_GUID_NOT_FOUND,             STATUS_WMI_GUID_NOT_FOUND },
    { ERROR_WMI_INSTANCE_NOT_FOUND,         STATUS_WMI_INSTANCE_NOT_FOUND },
    { ERROR_ACTIVE_CONNECTIONS,             STATUS_ALREADY_DISCONNECTED },
    { ERROR_CTX_CLOSE_PENDING,              STATUS_CTX_CLOSE_PENDING },
    { ERROR_SERVICES_FAILED_AUTOSTART,      STATUS_SERVICES_FAILED_AUTOSTART },
    { ERROR_INVALID_SERVICE_CONTROL,        STATUS_INVALID_DEVICE_REQUEST },
    { ERROR_MUI_FILE_NOT_FOUND,             STATUS_MUI_FILE_NOT_FOUND },
    { ERROR_MUI_INVALID_FILE,               STATUS_MUI_INVALID_FILE },
    { ERROR_MUI_INVALID_RC_CONFIG,          STATUS_MUI_INVALID_RC_CONFIG },
    { ERROR_MUI_INVALID_LOCALE_NAME,        STATUS_MUI_INVALID_LOCALE_NAME },
    { ERROR_MUI_INVALID_ULTIMATEFALLBACK_NAME, STATUS_MUI_INVALID_ULTIMATEFALLBACK_NAME },
    { ERROR_MUI_FILE_NOT_LOADED,            STATUS_MUI_FILE_NOT_LOADED },
    { ERROR_RESOURCE_ENUM_USER_STOP,        STATUS_RESOURCE_ENUM_USER_STOP },
    { NTE_INVALID_HANDLE,                   STATUS_INVALID_HANDLE },
    { NTE_INVALID_PARAMETER,                STATUS_INVALID_PARAMETER },
    { NTE_BUFFER_TOO_SMALL,                 STATUS_BUFFER_TOO_SMALL },
};


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
#if defined(PH_NATIVE_NTSTATUS_TO_DOS_ERROR)
    ULONG i;

    if (NT_NTWIN32(Status)) // RtlNtStatusToDosError doesn't seem to handle these cases correctly
        return WIN32_FROM_NTSTATUS(Status);

    for (i = 0; i < RTL_NUMBER_OF(PhpDosErrorStatusTable); i++)
    {
        if (PhpDosErrorStatusTable[i].Status == Status)
            return PhpDosErrorStatusTable[i].DosError;
    }

    return RtlNtStatusToDosErrorNoTeb(Status);
#else
    return RtlNtStatusToDosErrorNoTeb(Status);
#endif
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
    case STATUS_ACCOUNT_DISABLED: return ERROR_SERVICE_DISABLED;
    case STATUS_OBJECT_NAME_NOT_FOUND: return ERROR_SERVICE_DOES_NOT_EXIST;
    case STATUS_OBJECT_NAME_COLLISION: return ERROR_SERVICE_EXISTS;
    case STATUS_OBJECT_NAME_EXISTS: return ERROR_DUPLICATE_SERVICE_NAME;
    case STATUS_OBJECT_PATH_INVALID: return ERROR_SERVICE_NOT_FOUND;
    case STATUS_SERVICES_FAILED_AUTOSTART: return ERROR_SERVICES_FAILED_AUTOSTART;
    case STATUS_THREAD_NOT_RUNNING: return ERROR_SERVICE_NEVER_STARTED;
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
    ULONG i;

    if (NT_CUSTOMER(DosError) || NT_NTWIN32(DosError))
        return DosError;

    for (i = 0; i < RTL_NUMBER_OF(PhpDosErrorStatusTable); i++)
    {
        if (PhpDosErrorStatusTable[i].DosError == DosError)
            return PhpDosErrorStatusTable[i].Status;
    }

    assert(FALSE); // Update the table. (dmex)
    return NTSTATUS_FROM_WIN32(DosError);
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

/**
 * Determines whether a HRESULT value was converted from a NTSTATUS and returns the original error code.
 */
NTSTATUS PhNtStatusFromHResult(
    _In_ HRESULT Result
    )
{
    if (HRESULT_CUSTOMER(Result))
    {
        NOTHING;
    }
    else if (HRESULT_NTSTATUS(Result)) // if (FlagOn(Result, FACILITY_NT_BIT))
    {
        ClearFlag(Result, FACILITY_NT_BIT); // reverse HRESULT_FROM_NT (dmex)
    }
    else if (
        HRESULT_FACILITY(Result) == FACILITY_WIN32 ||
        HRESULT_FACILITY(Result) == FACILITY_WINDOWS
        )
    {
        Result = PhDosErrorToNtStatus(HRESULT_CODE(Result));
    }

    return Result;
}

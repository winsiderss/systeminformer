/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <apiimport.h>

 /**
  * Create a new job object in the object namespace.
  *
  * \param JobObject Receives the handle to the created job object on success.
  * \param DesiredAccess Desired access mask for the returned handle (e.g., JOB_OBJECT_ALL_ACCESS).
  * \param RootDirectory Optional handle to an object directory; if non-NULL the object is created relative to this directory.
  * \param ObjectName Pointer to a constant string reference that names the object. If NULL or empty, an unnamed job is created.
  * \return NTSTATUS Successful or errant status.
  */
NTSTATUS PhCreateJobObject(
    _Out_ PHANDLE JobObject,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_opt_ PCPH_STRINGREF ObjectName
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (ObjectName)
    {
        if (!PhStringRefToUnicodeString(ObjectName, &objectName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&objectName, NULL, 0);
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtCreateJobObject(
        JobObject,
        DesiredAccess,
        &objectAttributes
        );

    return status;
}

/*
 * Opens a job object.
 *
 * \param JobHandle A variable which receives a handle to the job object.
 * \param DesiredAccess The desired access to the job object.
 * \param RootDirectory A handle to the object directory of the job.
 * \param ObjectName The name of the job object.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenJobObject(
    _Out_ PHANDLE JobHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenJobObject(
        JobHandle,
        DesiredAccess,
        &objectAttributes
        );

    return status;
}

/**
 * Retrieves the list of process IDs associated with a specified job object.
 *
 * \param JobHandle A handle to the job object whose process ID list is to be retrieved.
 * \param ProcessIdList A pointer to a JOBOBJECT_BASIC_PROCESS_ID_LIST structure containing the list of process IDs.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetJobProcessIdList(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_PROCESS_ID_LIST *ProcessIdList
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    do
    {
        buffer = PhAllocate(bufferSize);

        status = NtQueryInformationJobObject(
            JobHandle,
            JobObjectBasicProcessIdList,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
        {
            *ProcessIdList = (PJOBOBJECT_BASIC_PROCESS_ID_LIST)buffer;
        }
        else
        {
            PhFree(buffer);
        }

    } while (status == STATUS_BUFFER_OVERFLOW);

    return status;
}

/**
 * Query basic and I/O accounting information for a job object.
 *
 * \param JobHandle Handle to the job object to query.
 * \param BasicAndIoAccounting Pointer to a JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION structure that receives the data.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetJobBasicAndIoAccounting(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION BasicAndIoAccounting
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectBasicAndIoAccountingInformation,
        BasicAndIoAccounting,
        sizeof(JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION),
        NULL
        );
}

/**
 * Query the basic limit information for a job object.
 *
 * \param JobHandle Handle to the job object to query.
 * \param BasicLimits Pointer to a JOBOBJECT_BASIC_LIMIT_INFORMATION structure that receives the data.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetJobBasicLimits(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimits
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectBasicLimitInformation,
        BasicLimits,
        sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION),
        NULL
        );
}

/**
 * Query extended limit information for a job object.
 *
 * \param JobHandle Handle to the job object to query.
 * \param ExtendedLimits Pointer to a JOBOBJECT_EXTENDED_LIMIT_INFORMATION structure that receives the data.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetJobExtendedLimits(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimits
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectExtendedLimitInformation,
        ExtendedLimits,
        sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),
        NULL
        );
}

/**
 * Query UI restrictions imposed by a job object.
 *
 * \param JobHandle Handle to the job object to query.
 * \param BasicUiRestrictions Pointer to a JOBOBJECT_BASIC_UI_RESTRICTIONS structure that receives the data.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetJobBasicUiRestrictions(
    _In_ HANDLE JobHandle,
    _Out_ PJOBOBJECT_BASIC_UI_RESTRICTIONS BasicUiRestrictions
    )
{
    return NtQueryInformationJobObject(
        JobHandle,
        JobObjectBasicUIRestrictions,
        BasicUiRestrictions,
        sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS),
        NULL
        );
}

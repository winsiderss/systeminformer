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
#include <kphuser.h>
#include <lsasup.h>
#include <mapldr.h>

#define PH_DEVICE_PREFIX_LENGTH 64
#define PH_DEVICE_MUP_PREFIX_MAX_COUNT 16

static PH_INITONCE PhDevicePrefixesInitOnce = PH_INITONCE_INIT;

static UNICODE_STRING PhDevicePrefixes[26];
static PH_QUEUED_LOCK PhDevicePrefixesLock = PH_QUEUED_LOCK_INIT;

static PPH_STRING PhDeviceMupPrefixes[PH_DEVICE_MUP_PREFIX_MAX_COUNT] = { 0 };
static ULONG PhDeviceMupPrefixesCount = 0;
static PH_QUEUED_LOCK PhDeviceMupPrefixesLock = PH_QUEUED_LOCK_INIT;

static PH_INITONCE PhPredefineKeyInitOnce = PH_INITONCE_INIT;
static UNICODE_STRING PhPredefineKeyNames[PH_KEY_MAXIMUM_PREDEFINE] =
{
    RTL_CONSTANT_STRING(L"\\Registry\\Machine"),
    RTL_CONSTANT_STRING(L"\\Registry\\User"),
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Classes"),
    { 0, 0, NULL }
};
static HANDLE PhPredefineKeyHandles[PH_KEY_MAXIMUM_PREDEFINE] = { 0 };

/**
 * Retrieves a copy of an object's security descriptor.
 *
 * \param Handle A handle to the object whose security descriptor is to be queried.
 * \param SecurityInformation The type of security information to be queried.
 * \param SecurityDescriptor A copy of the specified security descriptor in self-relative format.
 */
NTSTATUS PhGetObjectSecurity(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor
    )
{
    NTSTATUS status;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);
    // This is required (especially for File objects) because some drivers don't seem to handle
    // QuerySecurity properly. (wj32)
    memset(buffer, 0, bufferSize);

    status = NtQuerySecurityObject(
        Handle,
        SecurityInformation,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);
        memset(buffer, 0, bufferSize);

        status = NtQuerySecurityObject(
            Handle,
            SecurityInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *SecurityDescriptor = (PSECURITY_DESCRIPTOR)buffer;

    return status;
}

/**
 * Sets an object's security descriptor.
 *
 * \param Handle A handle to the object whose security descriptor is to be set.
 * \param SecurityInformation The type of security information to be set.
 * \param SecurityDescriptor The security descriptor in self-relative format.
 */
NTSTATUS PhSetObjectSecurity(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    return NtSetSecurityObject(
        Handle,
        SecurityInformation,
        SecurityDescriptor
        );
}

_Success_(return)
BOOLEAN PhEnumProcessEnvironmentVariables(
    _In_ PVOID Environment,
    _In_ ULONG EnvironmentLength,
    _Inout_ PULONG EnumerationKey,
    _Out_ PPH_ENVIRONMENT_VARIABLE Variable
    )
{
    ULONG length;
    ULONG startIndex;
    PWCHAR name;
    ULONG nameLength;
    PWCHAR value;
    ULONG valueLength;
    PWCHAR currentChar;
    ULONG currentIndex;

    length = EnvironmentLength / sizeof(WCHAR);

    currentIndex = *EnumerationKey;
    currentChar = PTR_ADD_OFFSET(Environment, currentIndex * sizeof(WCHAR));
    startIndex = currentIndex;
    name = currentChar;

    // Find the end of the name.
    while (TRUE)
    {
        if (currentIndex >= length)
            return FALSE;
        if (*currentChar == L'=' && startIndex != currentIndex)
            break; // equality sign is considered as a delimiter unless it is the first character (diversenok)
        if (*currentChar == UNICODE_NULL)
            return FALSE; // no more variables

        currentIndex++;
        currentChar++;
    }

    nameLength = currentIndex - startIndex;

    currentIndex++;
    currentChar++;
    startIndex = currentIndex;
    value = currentChar;

    // Find the end of the value.
    while (TRUE)
    {
        if (currentIndex >= length)
            return FALSE;
        if (*currentChar == UNICODE_NULL)
            break;

        currentIndex++;
        currentChar++;
    }

    valueLength = currentIndex - startIndex;

    currentIndex++;
    *EnumerationKey = currentIndex;

    Variable->Name.Buffer = name;
    Variable->Name.Length = nameLength * sizeof(WCHAR);
    Variable->Value.Buffer = value;
    Variable->Value.Length = valueLength * sizeof(WCHAR);

    return TRUE;
}

NTSTATUS PhQueryEnvironmentVariableStringRef(
    _In_opt_ PVOID Environment,
    _In_ PCPH_STRINGREF Name,
    _Inout_opt_ PPH_STRINGREF Value
    )
{
    NTSTATUS status;
    SIZE_T returnLength;

    status = RtlQueryEnvironmentVariable(
        Environment,
        Name->Buffer,
        Name->Length / sizeof(WCHAR),
        Value ? Value->Buffer : NULL,
        Value ? Value->Length / sizeof(WCHAR) : 0,
        &returnLength
        );

    if (Value && NT_SUCCESS(status))
    {
        Value->Length = returnLength * sizeof(WCHAR);
    }

    return status;
}

NTSTATUS PhQueryEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PCPH_STRINGREF Name,
    _Out_opt_ PPH_STRING* Value
    )
{
#ifdef PHNT_UNICODESTRING_ENVIRONMENTVARIABLE
    NTSTATUS status;
    UNICODE_STRING variableName;
    UNICODE_STRING variableValue;

    PhStringRefToUnicodeString(Name, &variableName);

    if (Value)
    {
        variableValue.Length = 0x100 * sizeof(WCHAR);
        variableValue.MaximumLength = variableValue.Length + sizeof(UNICODE_NULL);
        variableValue.Buffer = PhAllocate(variableValue.MaximumLength);
    }
    else
    {
        RtlInitEmptyUnicodeString(&variableValue, NULL, 0);
    }

    status = RtlQueryEnvironmentVariable_U(
        Environment,
        &variableName,
        &variableValue
        );

    if (Value && status == STATUS_BUFFER_TOO_SMALL)
    {
        if (variableValue.Length + sizeof(UNICODE_NULL) > UNICODE_STRING_MAX_BYTES)
            variableValue.MaximumLength = variableValue.Length;
        else
            variableValue.MaximumLength = variableValue.Length + sizeof(UNICODE_NULL);

        PhFree(variableValue.Buffer);
        variableValue.Buffer = PhAllocate(variableValue.MaximumLength);

        status = RtlQueryEnvironmentVariable_U(
            Environment,
            &variableName,
            &variableValue
            );
    }

    if (Value && NT_SUCCESS(status))
    {
        *Value = PhCreateStringFromUnicodeString(&variableValue);
    }

    if (Value && variableValue.Buffer)
    {
        PhFree(variableValue.Buffer);
    }

    return status;
#else
    NTSTATUS status;
    PPH_STRING buffer;
    SIZE_T returnLength;

    status = RtlQueryEnvironmentVariable(
        Environment,
        Name->Buffer,
        Name->Length / sizeof(WCHAR),
        NULL,
        0,
        &returnLength
        );

    if (Value && status == STATUS_BUFFER_TOO_SMALL)
    {
        buffer = PhCreateStringEx(NULL, returnLength * sizeof(WCHAR));

        status = RtlQueryEnvironmentVariable(
            Environment,
            Name->Buffer,
            Name->Length / sizeof(WCHAR),
            buffer->Buffer,
            buffer->Length / sizeof(WCHAR),
            &returnLength
            );

        if (NT_SUCCESS(status))
        {
            buffer->Length = returnLength * sizeof(WCHAR);
            *Value = buffer;
        }
        else
        {
            PhDereferenceObject(buffer);
        }
    }
    else
    {
        if (Value)
            *Value = NULL;
    }

    return status;
#endif
}

NTSTATUS PhSetEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PCPH_STRINGREF Name,
    _In_opt_ PCPH_STRINGREF Value
    )
{
    NTSTATUS status;
    UNICODE_STRING variableName;
    UNICODE_STRING variableValue;

    PhStringRefToUnicodeString(Name, &variableName);

    if (Value)
        PhStringRefToUnicodeString(Value, &variableValue);
    else
        RtlInitEmptyUnicodeString(&variableValue, NULL, 0);

    status = RtlSetEnvironmentVariable(
        Environment,
        &variableName,
        &variableValue
        );

    return status;
}

NTSTATUS PhGetProcessSectionFileName(
    _In_ HANDLE SectionHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    SIZE_T viewSize;
    PVOID viewBase;

    viewSize = PAGE_SIZE;
    viewBase = NULL;

    status = NtMapViewOfSection(
        SectionHandle,
        ProcessHandle,
        &viewBase,
        0,
        0,
        NULL,
        &viewSize,
        ViewUnmap,
        0,
        PAGE_READONLY
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcessMappedFileName(
        ProcessHandle,
        viewBase,
        FileName
        );

    NtUnmapViewOfSection(ProcessHandle, viewBase);

    return status;
}

NTSTATUS PhGetProcessUnloadedDlls(
    _In_ HANDLE ProcessId,
    _Out_ PVOID *EventTrace,
    _Out_ ULONG *EventTraceSize,
    _Out_ ULONG *EventTraceCount
    )
{
    NTSTATUS status;
    PULONG elementSize;
    PULONG elementCount;
    PVOID eventTrace;
    HANDLE processHandle = NULL;
    ULONG eventTraceSize;
    ULONG capturedElementSize = 0;
    ULONG capturedElementCount = 0;
    PVOID capturedEventTracePointer;
    PVOID capturedEventTrace = NULL;

    RtlGetUnloadEventTraceEx(&elementSize, &elementCount, &eventTrace);

    if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_VM_READ, ProcessId)))
        goto CleanupExit;

    // We have the pointers for the unload event trace information.
    // Since ntdll is loaded at the same base address across all processes,
    // we can read the information in.

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        processHandle,
        elementSize,
        &capturedElementSize,
        sizeof(ULONG),
        NULL
        )))
        goto CleanupExit;

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        processHandle,
        elementCount,
        &capturedElementCount,
        sizeof(ULONG),
        NULL
        )))
        goto CleanupExit;

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        processHandle,
        eventTrace,
        &capturedEventTracePointer,
        sizeof(PVOID),
        NULL
        )))
        goto CleanupExit;

    if (!capturedEventTracePointer)
    {
        status = STATUS_NOT_FOUND; // no events
        goto CleanupExit;
    }

    if (capturedElementCount > 0x4000)
        capturedElementCount = 0x4000;

    eventTraceSize = capturedElementSize * capturedElementCount;
    capturedEventTrace = PhAllocateSafe(eventTraceSize);

    if (!capturedEventTrace)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        processHandle,
        capturedEventTracePointer,
        capturedEventTrace,
        eventTraceSize,
        NULL
        )))
        goto CleanupExit;

CleanupExit:

    if (processHandle)
        NtClose(processHandle);

    if (NT_SUCCESS(status))
    {
        *EventTrace = capturedEventTrace;
        *EventTraceSize = capturedElementSize;
        *EventTraceCount = capturedElementCount;
    }
    else
    {
        if (capturedEventTrace)
            PhFree(capturedEventTrace);
    }

    return status;
}

NTSTATUS PhTraceControl(
    _In_ ETWTRACECONTROLCODE TraceInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;

    status = NtTraceControl(
        TraceInformationClass,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength,
        ReturnLength
        );

    return status;
}

NTSTATUS PhTraceControlVariableSize(
    _In_ ETWTRACECONTROLCODE TraceInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(*OutputBufferLength) PVOID* OutputBuffer,
    _Out_opt_ PULONG OutputBufferLength
    )
{
    NTSTATUS status;
    PVOID buffer = NULL;
    ULONG bufferLength = 0;
    ULONG returnLength = 0;

    status = NtTraceControl(
        TraceInformationClass,
        InputBuffer,
        InputBufferLength,
        NULL,
        0,
        &returnLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        bufferLength = returnLength;
        buffer = PhAllocate(bufferLength);

        status = NtTraceControl(
            TraceInformationClass,
            InputBuffer,
            InputBufferLength,
            buffer,
            bufferLength,
            &bufferLength
            );
    }

    if (NT_SUCCESS(status))
    {
        if (OutputBuffer)
            *OutputBuffer = buffer;
        if (OutputBufferLength)
            *OutputBufferLength = bufferLength;
    }
    else
    {
        if (buffer)
            PhFree(buffer);
    }

    return status;
}

NTSTATUS PhGetWindowClientId(
    _In_ HWND WindowHandle,
    _Out_ PCLIENT_ID ClientId
    )
{
    ULONG windowProcessId;
    ULONG windowThreadId;

    if (windowThreadId = GetWindowThreadProcessId(WindowHandle, &windowProcessId))
    {
        ClientId->UniqueProcess = UlongToHandle(windowProcessId);
        ClientId->UniqueThread = UlongToHandle(windowThreadId);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

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

NTSTATUS PhpQueryTransactionManagerVariableSize(
    _In_ HANDLE TransactionManagerHandle,
    _In_ TRANSACTIONMANAGER_INFORMATION_CLASS TransactionManagerInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    if (!NtQueryInformationTransactionManager_Import())
        return STATUS_NOT_SUPPORTED;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationTransactionManager_Import()(
            TransactionManagerHandle,
            TransactionManagerInformationClass,
            buffer,
            bufferSize,
            NULL
            );

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            bufferSize *= 2;

            if (bufferSize > 1 * 1024 * 1024)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhGetTransactionManagerBasicInformation(
    _In_ HANDLE TransactionManagerHandle,
    _Out_ PTRANSACTIONMANAGER_BASIC_INFORMATION BasicInformation
    )
{
    memset(BasicInformation, 0, sizeof(TRANSACTIONMANAGER_BASIC_INFORMATION));

    if (NtQueryInformationTransactionManager_Import())
    {
        return NtQueryInformationTransactionManager_Import()(
            TransactionManagerHandle,
            TransactionManagerBasicInformation,
            BasicInformation,
            sizeof(TRANSACTIONMANAGER_BASIC_INFORMATION),
            NULL
            );
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS PhGetTransactionManagerLogFileName(
    _In_ HANDLE TransactionManagerHandle,
    _Out_ PPH_STRING *LogFileName
    )
{
    NTSTATUS status;
    PTRANSACTIONMANAGER_LOGPATH_INFORMATION logPathInfo;

    status = PhpQueryTransactionManagerVariableSize(
        TransactionManagerHandle,
        TransactionManagerLogPathInformation,
        &logPathInfo
        );

    if (!NT_SUCCESS(status))
        return status;

    if (logPathInfo->LogPathLength == 0)
    {
        *LogFileName = PhReferenceEmptyString();
    }
    else
    {
        *LogFileName = PhCreateStringEx(
            logPathInfo->LogPath,
            logPathInfo->LogPathLength
            );
    }

    PhFree(logPathInfo);

    return status;
}

NTSTATUS PhpQueryTransactionVariableSize(
    _In_ HANDLE TransactionHandle,
    _In_ TRANSACTION_INFORMATION_CLASS TransactionInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    if (!NtQueryInformationTransaction_Import())
        return STATUS_NOT_SUPPORTED;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationTransaction_Import()(
            TransactionHandle,
            TransactionInformationClass,
            buffer,
            bufferSize,
            NULL
            );

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            bufferSize *= 2;

            if (bufferSize > 1 * 1024 * 1024)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhGetTransactionBasicInformation(
    _In_ HANDLE TransactionHandle,
    _Out_ PTRANSACTION_BASIC_INFORMATION BasicInformation
    )
{
    memset(BasicInformation, 0, sizeof(TRANSACTION_BASIC_INFORMATION));

    if (NtQueryInformationTransaction_Import())
    {
        return NtQueryInformationTransaction_Import()(
            TransactionHandle,
            TransactionBasicInformation,
            BasicInformation,
            sizeof(TRANSACTION_BASIC_INFORMATION),
            NULL
            );
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS PhGetTransactionPropertiesInformation(
    _In_ HANDLE TransactionHandle,
    _Out_opt_ PLARGE_INTEGER Timeout,
    _Out_opt_ TRANSACTION_OUTCOME *Outcome,
    _Out_opt_ PPH_STRING *Description
    )
{
    NTSTATUS status;
    PTRANSACTION_PROPERTIES_INFORMATION propertiesInfo;

    status = PhpQueryTransactionVariableSize(
        TransactionHandle,
        TransactionPropertiesInformation,
        &propertiesInfo
        );

    if (!NT_SUCCESS(status))
        return status;

    if (Timeout)
    {
        *Timeout = propertiesInfo->Timeout;
    }

    if (Outcome)
    {
        *Outcome = propertiesInfo->Outcome;
    }

    if (Description)
    {
        *Description = PhCreateStringEx(
            propertiesInfo->Description,
            propertiesInfo->DescriptionLength
            );
    }

    PhFree(propertiesInfo);

    return status;
}

NTSTATUS PhpQueryResourceManagerVariableSize(
    _In_ HANDLE ResourceManagerHandle,
    _In_ RESOURCEMANAGER_INFORMATION_CLASS ResourceManagerInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x100;

    if (!NtQueryInformationResourceManager_Import())
        return STATUS_NOT_SUPPORTED;

    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtQueryInformationResourceManager_Import()(
            ResourceManagerHandle,
            ResourceManagerInformationClass,
            buffer,
            bufferSize,
            NULL
            );

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            bufferSize *= 2;

            if (bufferSize > 1 * 1024 * 1024)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhGetResourceManagerBasicInformation(
    _In_ HANDLE ResourceManagerHandle,
    _Out_opt_ PGUID Guid,
    _Out_opt_ PPH_STRING *Description
    )
{
    NTSTATUS status;
    PRESOURCEMANAGER_BASIC_INFORMATION basicInfo;

    status = PhpQueryResourceManagerVariableSize(
        ResourceManagerHandle,
        ResourceManagerBasicInformation,
        &basicInfo
        );

    if (!NT_SUCCESS(status))
        return status;

    if (Guid)
    {
        *Guid = basicInfo->ResourceManagerId;
    }

    if (Description)
    {
        if (basicInfo->DescriptionLength == 0)
        {
            *Description = PhReferenceEmptyString();
        }
        else
        {
            *Description = PhCreateStringEx(
                basicInfo->Description,
                basicInfo->DescriptionLength
                );
        }
    }

    PhFree(basicInfo);

    return status;
}

NTSTATUS PhGetEnlistmentBasicInformation(
    _In_ HANDLE EnlistmentHandle,
    _Out_ PENLISTMENT_BASIC_INFORMATION BasicInformation
    )
{
    memset(BasicInformation, 0, sizeof(ENLISTMENT_BASIC_INFORMATION));

    if (NtQueryInformationEnlistment_Import())
    {
        return NtQueryInformationEnlistment_Import()(
            EnlistmentHandle,
            EnlistmentBasicInformation,
            BasicInformation,
            sizeof(ENLISTMENT_BASIC_INFORMATION),
            NULL
            );
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }
}

typedef struct _OPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT
{
    NTSTATUS Status;
    PVOID BaseAddress;
    HANDLE DriverHandle;
} OPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT, *POPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT;

_Function_class_(PH_ENUM_DIRECTORY_OBJECTS)
BOOLEAN NTAPI PhpOpenDriverByBaseAddressCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ POPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE driverHandle;
    UNICODE_STRING driverName;
    OBJECT_ATTRIBUTES objectAttributes;
    KPH_DRIVER_BASIC_INFORMATION basicInfo;

    if (!PhStringRefToUnicodeString(Name, &driverName))
        return TRUE;

    InitializeObjectAttributes(
        &objectAttributes,
        &driverName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = KphOpenDriver(
        &driverHandle,
        SYNCHRONIZE,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        return TRUE;

    status = KphQueryInformationDriver(
        driverHandle,
        KphDriverBasicInformation,
        &basicInfo,
        sizeof(KPH_DRIVER_BASIC_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (basicInfo.DriverStart == Context->BaseAddress)
        {
            Context->Status = STATUS_SUCCESS;
            Context->DriverHandle = driverHandle;

            return FALSE;
        }
    }

    NtClose(driverHandle);

    return TRUE;
}

/**
 * Opens a driver object using a base address.
 *
 * \param DriverHandle A variable which receives a handle to the driver object.
 * \param BaseAddress The base address of the driver to open.
 *
 * \retval STATUS_OBJECT_NAME_NOT_FOUND The driver could not be found.
 *
 * \remarks This function requires a valid KSystemInformer handle.
 */
NTSTATUS PhOpenDriverByBaseAddress(
    _Out_ PHANDLE DriverHandle,
    _In_ PVOID BaseAddress
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    UNICODE_STRING objectDirectory;
    OBJECT_ATTRIBUTES objectAttributes;
    OPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT context;

    RtlInitUnicodeString(&objectDirectory, L"\\Driver");
    InitializeObjectAttributes(
        &objectAttributes,
        &objectDirectory,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        return status;

    context.Status = STATUS_OBJECT_NAME_NOT_FOUND;
    context.BaseAddress = BaseAddress;

    status = PhEnumDirectoryObjects(
        directoryHandle,
        PhpOpenDriverByBaseAddressCallback,
        &context
        );

    NtClose(directoryHandle);

    if (NT_SUCCESS(status))
    {
        if (NT_SUCCESS(context.Status))
        {
            *DriverHandle = context.DriverHandle;
            return STATUS_SUCCESS;
        }

        return context.Status;
    }

    return status;
}

NTSTATUS PhOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName
    )
{
    if (KsiLevel() == KphLevelMax)
    {
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

        return KphOpenDriver(
            DriverHandle,
            DesiredAccess,
            &objectAttributes
            );
    }
    else
    {
        return STATUS_NOT_IMPLEMENTED;
    }
}

/**
 * Queries variable-sized information for a driver. The function allocates a buffer to contain the
 * information.
 *
 * \param DriverHandle A handle to a driver. The access required depends on the information class
 * specified.
 * \param DriverInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer containing the information. You
 * must free the buffer using PhFree() when you no longer need it.
 *
 * \remarks This function requires a valid KSystemInformer handle.
 */
NTSTATUS PhpQueryDriverVariableSize(
    _In_ HANDLE DriverHandle,
    _In_ KPH_DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer = NULL;
    ULONG bufferSize;
    ULONG returnLength = 0;

    status = KphQueryInformationDriver(
        DriverHandle,
        DriverInformationClass,
        NULL,
        0,
        &returnLength
        );

    if (NT_SUCCESS(status) && returnLength == 0)
    {
        return STATUS_NOT_FOUND;
    }

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = KphQueryInformationDriver(
            DriverHandle,
            DriverInformationClass,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        if (returnLength == 0)
        {
            status = STATUS_NOT_FOUND;
            PhFree(buffer);
        }
        else
        {
            *Buffer = buffer;
        }
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

/**
 * Gets the object name of a driver.
 *
 * \param DriverHandle A handle to a driver.
 * \param Name A variable which receives a pointer to a string containing the object name. You must
 * free the string using PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function requires a valid KSystemInformer handle.
 */
NTSTATUS PhGetDriverName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *Name
    )
{
    NTSTATUS status;
    PUNICODE_STRING unicodeString;

    if (!NT_SUCCESS(status = PhpQueryDriverVariableSize(
        DriverHandle,
        KphDriverNameInformation,
        &unicodeString
        )))
        return status;

    *Name = PhCreateStringFromUnicodeString(unicodeString);
    PhFree(unicodeString);

    return status;
}

/**
 * Gets the object name of a driver.
 *
 * \param DriverHandle A handle to a driver.
 * \param Name A variable which receives a pointer to a string containing the driver image file name.
 * You must free the string using PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function requires a valid KSystemInformer handle.
 */
NTSTATUS PhGetDriverImageFileName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *Name
    )
{
    NTSTATUS status;
    PUNICODE_STRING unicodeString;

    if (!NT_SUCCESS(status = PhpQueryDriverVariableSize(
        DriverHandle,
        KphDriverImageFileNameInformation,
        &unicodeString
        )))
        return status;

    *Name = PhCreateStringFromUnicodeString(unicodeString);
    PhFree(unicodeString);

    return status;
}

/**
 * Gets the service key name of a driver.
 *
 * \param DriverHandle A handle to a driver.
 * \param ServiceKeyName A variable which receives a pointer to a string containing the service key
 * name. You must free the string using PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function requires a valid KSystemInformer handle.
 */
NTSTATUS PhGetDriverServiceKeyName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *ServiceKeyName
    )
{
    NTSTATUS status;
    PUNICODE_STRING unicodeString;

    if (!NT_SUCCESS(status = PhpQueryDriverVariableSize(
        DriverHandle,
        KphDriverServiceKeyNameInformation,
        &unicodeString
        )))
        return status;

    *ServiceKeyName = PhCreateStringFromUnicodeString(unicodeString);
    PhFree(unicodeString);

    return status;
}

NTSTATUS PhpUnloadDriver(
    _In_ PCPH_STRINGREF ServiceKeyName,
    _In_ PCPH_STRINGREF DriverFileName
    )
{
    static CONST PH_STRINGREF fullServicesKeyName = PH_STRINGREF_INIT(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    NTSTATUS status;
    PPH_STRING fullServiceKeyName;
    UNICODE_STRING fullServiceKeyNameUs;
    HANDLE serviceKeyHandle;
    ULONG disposition;

    fullServiceKeyName = PhConcatStringRef2(&fullServicesKeyName, ServiceKeyName);

    if (!PhStringRefToUnicodeString(&fullServiceKeyName->sr, &fullServiceKeyNameUs))
    {
        PhDereferenceObject(fullServiceKeyName);
        return STATUS_NAME_TOO_LONG;
    }

    if (NT_SUCCESS(status = PhCreateKey(
        &serviceKeyHandle,
        KEY_WRITE | DELETE,
        NULL,
        &fullServiceKeyName->sr,
        0,
        0,
        &disposition
        )))
    {
        if (disposition == REG_CREATED_NEW_KEY)
        {
            ULONG regValue = 0;

            // Set up the required values.
            PhSetValueKeyZ(serviceKeyHandle, L"ErrorControl", REG_DWORD, &regValue, sizeof(ULONG));
            PhSetValueKeyZ(serviceKeyHandle, L"Start", REG_DWORD, &regValue, sizeof(ULONG));
            PhSetValueKeyZ(serviceKeyHandle, L"Type", REG_DWORD, &regValue, sizeof(ULONG));
            PhSetValueKeyZ(serviceKeyHandle, L"ImagePath", REG_SZ, DriverFileName->Buffer, (ULONG)DriverFileName->Length + sizeof(UNICODE_NULL));
        }

        status = NtUnloadDriver(&fullServiceKeyNameUs);

        if (disposition == REG_CREATED_NEW_KEY)
        {
            // We added values, not subkeys, so this function will work correctly.
            NtDeleteKey(serviceKeyHandle);
        }

        NtClose(serviceKeyHandle);
    }

    PhDereferenceObject(fullServiceKeyName);

    return status;
}

/**
 * Unloads a driver.
 *
 * \param BaseAddress The base address of the driver. This parameter can be NULL if a value is
 * specified in \c Name.
 * \param Name The base name of the driver. This parameter can be NULL if a value is specified in
 * \c BaseAddress and KSystemInformer is loaded.
 *
 * \retval STATUS_INVALID_PARAMETER_MIX Both \c BaseAddress and \c Name were null, or \c Name was
 * not specified and KSystemInformer is not loaded.
 * \retval STATUS_OBJECT_NAME_NOT_FOUND The driver could not be found.
 */
NTSTATUS PhUnloadDriver(
    _In_opt_ PVOID BaseAddress,
    _In_opt_ PCPH_STRINGREF Name,
    _In_opt_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    HANDLE driverHandle;
    PPH_STRING serviceKeyName = NULL;
    KPH_LEVEL level;

    level = KsiLevel();

    if (!BaseAddress && !Name)
        return STATUS_INVALID_PARAMETER_MIX;
    if (!Name && (level != KphLevelMax))
        return STATUS_INVALID_PARAMETER_MIX;

    // Try to get the service key name by scanning the Driver directory.

    if ((level == KphLevelMax) && BaseAddress)
    {
        if (!PhIsRtlModuleBase(BaseAddress))
        {
            if (NT_SUCCESS(PhOpenDriverByBaseAddress(
                &driverHandle,
                BaseAddress
                )))
            {
                PhGetDriverServiceKeyName(driverHandle, &serviceKeyName);
                NtClose(driverHandle);
            }
        }
    }

    // Use the base name if we didn't get the service key name.

    if (!serviceKeyName && Name)
    {
        PPH_STRING name;

        name = PhCreateString2(Name);

        // Remove the extension if it is present.
        if (PhEndsWithString2(name, L".sys", TRUE))
        {
            serviceKeyName = PhSubstring(name, 0, name->Length / sizeof(WCHAR) - 4);
            PhDereferenceObject(name);
        }
        else
        {
            serviceKeyName = name;
        }
    }

    if (!serviceKeyName)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    status = PhpUnloadDriver(&serviceKeyName->sr, FileName);
    PhDereferenceObject(serviceKeyName);

    return status;
}

/**
 * Enumerates the running processes.
 *
 * \param Processes A variable which receives a pointer to a buffer containing process information.
 * You must free the buffer using PhFree() when you no longer need it.
 *
 * \remarks You can use the \ref PH_FIRST_PROCESS and \ref PH_NEXT_PROCESS macros to process the
 * information contained in the buffer.
 */
NTSTATUS PhEnumProcesses(
    _Out_ PVOID *Processes
    )
{
    return PhEnumProcessesEx(Processes, SystemProcessInformation);
}

/**
 * Enumerates the running processes.
 *
 * \param Processes A variable which receives a pointer to a buffer containing process information.
 * You must free the buffer using PhFree() when you no longer need it.
 * \param SystemInformationClass A variable which indicates the kind of system information to be retrieved.
 *
 * \remarks You can use the \ref PH_FIRST_PROCESS and \ref PH_NEXT_PROCESS macros to process the
 * information contained in the buffer.
 */
NTSTATUS PhEnumProcessesEx(
    _Out_ PVOID *Processes,
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass
    )
{
    static ULONG initialBufferSize[3] = { 0x4000, 0x4000, 0x4000 };
    NTSTATUS status;
    ULONG classIndex;
    PVOID buffer;
    ULONG bufferSize;

    switch (SystemInformationClass)
    {
    case SystemProcessInformation:
        classIndex = 0;
        break;
    case SystemExtendedProcessInformation:
        classIndex = 1;
        break;
    case SystemFullProcessInformation:
        classIndex = 2;
        break;
    default:
        return STATUS_INVALID_INFO_CLASS;
    }

    bufferSize = initialBufferSize[classIndex];
    buffer = PhAllocateSafe(bufferSize);
    if (!buffer) return STATUS_NO_MEMORY;

    while (TRUE)
    {
        status = NtQuerySystemInformation(
            SystemInformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocateSafe(bufferSize);
            if (!buffer) return STATUS_NO_MEMORY;
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x100000) initialBufferSize[classIndex] = bufferSize;
    *Processes = buffer;

    return status;
}

/**
 * Enumerates the next process.
 *
 * \param ProcessHandle The handle to the current process. Pass NULL to start enumeration from the beginning.
 * \param DesiredAccess The desired access rights for the process handle.
 * \param Callback The callback function to be called for each enumerated process.
 * \param Context An optional context parameter to be passed to the callback function.
 *
 * \return Returns the status of the enumeration operation.
 *         If the enumeration is successful, it returns STATUS_SUCCESS.
 *         If there are no more processes to enumerate, it returns STATUS_NO_MORE_ENTRIES.
 *         Otherwise, it returns an appropriate NTSTATUS error code.
 */
NTSTATUS PhEnumNextProcess(
    _In_opt_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_ENUM_NEXT_PROCESS Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE newProcessHandle;

    status = NtGetNextProcess(
        ProcessHandle,
        DesiredAccess,
        0,
        0,
        &processHandle
        );

    if (!NT_SUCCESS(status))
        return status;

    while (TRUE)
    {
        status = Callback(processHandle, Context);

        if (status == STATUS_NO_MORE_ENTRIES)
            break;

        if (!NT_SUCCESS(status))
        {
            NtClose(processHandle);
            break;
        }

        status = NtGetNextProcess(
            processHandle,
            DesiredAccess,
            0,
            0,
            &newProcessHandle
            );

        if (NT_SUCCESS(status))
        {
            NtClose(processHandle);
            processHandle = newProcessHandle;
        }
        else
        {
            NtClose(processHandle);
            break;
        }
    }

    if (status == STATUS_NO_MORE_ENTRIES)
        status = STATUS_SUCCESS;

    return status;
}

/**
 * Enumerates the next thread.
 *
 * \param ProcessHandle The handle to the process.
 * \param ThreadHandle The handle to the current thread. Pass NULL to start enumeration from the beginning.
 * \param DesiredAccess The desired access rights for the thread handle.
 * \param Callback The callback function to be called for each enumerated thread.
 * \param Context An optional context parameter to be passed to the callback function.
 *
 * \return Returns the status of the enumeration operation.
 *         If the enumeration is successful, it returns STATUS_SUCCESS.
 *         If there are no more threads to enumerate, it returns STATUS_NO_MORE_ENTRIES.
 *         Otherwise, it returns an appropriate NTSTATUS error code.
 */
NTSTATUS PhEnumNextThread(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_ENUM_NEXT_THREAD Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    HANDLE newThreadHandle;

    status = NtGetNextThread(
        ProcessHandle,
        ThreadHandle,
        DesiredAccess,
        0,
        0,
        &threadHandle
        );

    if (!NT_SUCCESS(status))
        return status;

    while (TRUE)
    {
        status = Callback(threadHandle, Context);

        if (status == STATUS_NO_MORE_ENTRIES)
            break;

        if (!NT_SUCCESS(status))
        {
            NtClose(threadHandle);
            break;
        }

        status = NtGetNextThread(
            ProcessHandle,
            threadHandle,
            DesiredAccess,
            0,
            0,
            &newThreadHandle
            );

        if (NT_SUCCESS(status))
        {
            NtClose(threadHandle);
            threadHandle = newThreadHandle;
        }
        else
        {
            NtClose(threadHandle);
            break;
        }
    }

    if (status == STATUS_NO_MORE_ENTRIES)
        status = STATUS_SUCCESS;

    return status;
}

/**
 * Enumerates the running processes for a session.
 *
 * \param Processes A variable which receives a pointer to a buffer containing process information.
 * You must free the buffer using PhFree() when you no longer need it.
 * \param SessionId A session ID.
 *
 * \remarks You can use the \ref PH_FIRST_PROCESS and \ref PH_NEXT_PROCESS macros to process the
 * information contained in the buffer.
 */
NTSTATUS PhEnumProcessesForSession(
    _Out_ PVOID *Processes,
    _In_ ULONG SessionId
    )
{
    static ULONG initialBufferSize = 0x4000;
    NTSTATUS status;
    SYSTEM_SESSION_PROCESS_INFORMATION sessionProcessInfo;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    sessionProcessInfo.SessionId = SessionId;

    while (TRUE)
    {
        sessionProcessInfo.SizeOfBuf = bufferSize;
        sessionProcessInfo.Buffer = buffer;

        status = NtQuerySystemInformation(
            SystemSessionProcessInformation,
            &sessionProcessInfo,
            sizeof(SYSTEM_SESSION_PROCESS_INFORMATION),
            &bufferSize // size of the inner buffer gets returned
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x100000) initialBufferSize = bufferSize;
    *Processes = buffer;

    return status;
}

/**
 * Finds the process information structure for a specific process.
 *
 * \param Processes A pointer to a buffer returned by PhEnumProcesses().
 * \param ProcessId The ID of the process.
 *
 * \return A pointer to the process information structure for the specified process, or NULL if the
 * structure could not be found.
 */
PSYSTEM_PROCESS_INFORMATION PhFindProcessInformation(
    _In_ PVOID Processes,
    _In_ HANDLE ProcessId
    )
{
    PSYSTEM_PROCESS_INFORMATION process;

    process = PH_FIRST_PROCESS(Processes);

    do
    {
        if (process->UniqueProcessId == ProcessId)
            return process;
    } while (process = PH_NEXT_PROCESS(process));

    return NULL;
}

/**
 * Finds the process information structure for a specific process.
 *
 * \param Processes A pointer to a buffer returned by PhEnumProcesses().
 * \param ImageName The image name to search for.
 *
 * \return A pointer to the process information structure for the specified process, or NULL if the
 * structure could not be found.
 */
PSYSTEM_PROCESS_INFORMATION PhFindProcessInformationByImageName(
    _In_ PVOID Processes,
    _In_ PCPH_STRINGREF ImageName
    )
{
    PSYSTEM_PROCESS_INFORMATION process;
    PH_STRINGREF processImageName;

    process = PH_FIRST_PROCESS(Processes);

    do
    {
        PhUnicodeStringToStringRef(&process->ImageName, &processImageName);

        if (PhEqualStringRef(&processImageName, ImageName, TRUE))
            return process;
    } while (process = PH_NEXT_PROCESS(process));

    return NULL;
}

/**
 * Enumerates all open handles.
 *
 * \param Handles A variable which receives a pointer to a structure containing information about
 * all opened handles. You must free the structure using PhFree() when you no longer need it.
 *
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 */
NTSTATUS PhEnumHandles(
    _Out_ PSYSTEM_HANDLE_INFORMATION *Handles
    )
{
    static ULONG initialBufferSize = 0x10000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(
        SystemHandleInformation,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x200000) initialBufferSize = bufferSize;
    *Handles = (PSYSTEM_HANDLE_INFORMATION)buffer;

    return status;
}

/**
 * Enumerates all open handles.
 *
 * \param Handles A variable which receives a pointer to a structure containing information about
 * all opened handles. You must free the structure using PhFree() when you no longer need it.
 *
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 *
 * \remarks This function is only available starting with Windows XP.
 */
NTSTATUS PhEnumHandlesEx(
    _Out_ PSYSTEM_HANDLE_INFORMATION_EX *Handles
    )
{
    static ULONG initialBufferSize = 0x10000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength = 0;
    ULONG attempts = 0;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemExtendedHandleInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 10)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemExtendedHandleInformation,
            buffer,
            bufferSize,
            &returnLength
            );

        attempts++;
    }

    if (!NT_SUCCESS(status))
    {
        // Fall back to using the previous code that we've used since Windows XP (dmex)
        bufferSize = initialBufferSize;
        buffer = PhAllocate(bufferSize);

        while ((status = NtQuerySystemInformation(
            SystemExtendedHandleInformation,
            buffer,
            bufferSize,
            NULL
            )) == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            bufferSize *= 2;

            // Fail if we're resizing the buffer to something very large.
            if (bufferSize > PH_LARGE_BUFFER_SIZE)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferSize);
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x200000) initialBufferSize = bufferSize;
    *Handles = (PSYSTEM_HANDLE_INFORMATION_EX)buffer;

    return status;
}

/**
 * Enumerates all open handles.
 *
 * \param ProcessHandle A handle to the process. The handle must have PROCESS_QUERY_INFORMATION access.
 * \param Handles A variable which receives a pointer to a structure containing information about
 * handles opened by the process. You must free the structure using PhFree() when you no longer need it.
 *
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 *
 * \remarks This function is only available starting with Windows 8.
 */
NTSTATUS PhEnumProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_HANDLE_SNAPSHOT_INFORMATION *Handles
    )
{
    NTSTATUS status;
    PPROCESS_HANDLE_SNAPSHOT_INFORMATION buffer;
    ULONG bufferSize;
    ULONG returnLength = 0;
    ULONG attempts = 0;

    bufferSize = 0x8000;
    buffer = PhAllocatePage(bufferSize, NULL);
    buffer->NumberOfHandles = 0;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessHandleInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFreePage(buffer);
        bufferSize = returnLength;
        buffer = PhAllocatePage(bufferSize, NULL);
        buffer->NumberOfHandles = 0;

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessHandleInformation,
            buffer,
            bufferSize,
            &returnLength
            );

        attempts++;
    }

    if (NT_SUCCESS(status))
    {
        // NOTE: This is needed to workaround minimal processes on Windows 10
        // returning STATUS_SUCCESS with invalid handle data. (dmex)
        // NOTE: 21H1 and above no longer set NumberOfHandles to zero before returning
        // STATUS_SUCCESS so we first zero the entire buffer using PhAllocateZero. (dmex)
        if (buffer->NumberOfHandles == 0)
        {
            status = STATUS_UNSUCCESSFUL;
            PhFreePage(buffer);
        }
        else
        {
            *Handles = buffer;
        }
    }
    else
    {
        PhFreePage(buffer);
    }

    return status;
}

/**
 * Enumerates all handles in a process.
 *
 * \param ProcessId The ID of the process.
 * \param ProcessHandle A handle to the process.
 * \param EnableHandleSnapshot TRUE to return a snapshot of the process handles.
 * \param Handles A variable which receives a pointer to a buffer containing
 * information about the handles.
 */
NTSTATUS PhEnumHandlesGeneric(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN EnableHandleSnapshot,
    _Out_ PSYSTEM_HANDLE_INFORMATION_EX *Handles
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // There are three ways of enumerating handles:
    // * On Windows 8 and later, NtQueryInformationProcess with ProcessHandleInformation is the most efficient method.
    // * On Windows XP and later, NtQuerySystemInformation with SystemExtendedHandleInformation.
    // * Otherwise, NtQuerySystemInformation with SystemHandleInformation can be used.

    if ((KsiLevel() >= KphLevelMed) && ProcessHandle)
    {
        PKPH_PROCESS_HANDLE_INFORMATION handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i;

        // Enumerate handles using KSystemInformer. Unlike with NtQuerySystemInformation,
        // this only enumerates handles for a single process and saves a lot of processing.

        if (NT_SUCCESS(status = KsiEnumerateProcessHandles(ProcessHandle, &handles)))
        {
            convertedHandles = PhAllocate(UFIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[handles->HandleCount]));
            convertedHandles->NumberOfHandles = handles->HandleCount;

            for (i = 0; i < handles->HandleCount; i++)
            {
                PKPH_PROCESS_HANDLE handle = &handles->Handles[i];

                convertedHandles->Handles[i].Object = handle->Object;
                convertedHandles->Handles[i].UniqueProcessId = ProcessId;
                convertedHandles->Handles[i].HandleValue = handle->Handle;
                convertedHandles->Handles[i].GrantedAccess = handle->GrantedAccess;
                convertedHandles->Handles[i].CreatorBackTraceIndex = 0;
                convertedHandles->Handles[i].ObjectTypeIndex = handle->ObjectTypeIndex;
                convertedHandles->Handles[i].HandleAttributes = handle->HandleAttributes;
            }

            PhFree(handles);

            *Handles = convertedHandles;
        }
    }

    if (!NT_SUCCESS(status) && WindowsVersion >= WINDOWS_8 && ProcessHandle && EnableHandleSnapshot)
    {
        PPROCESS_HANDLE_SNAPSHOT_INFORMATION handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i;

        if (NT_SUCCESS(status = PhEnumProcessHandles(ProcessHandle, &handles)))
        {
            convertedHandles = PhAllocate(UFIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[handles->NumberOfHandles]));
            convertedHandles->NumberOfHandles = handles->NumberOfHandles;

            for (i = 0; i < handles->NumberOfHandles; i++)
            {
                PPROCESS_HANDLE_TABLE_ENTRY_INFO handle = &handles->Handles[i];

                convertedHandles->Handles[i].Object = NULL;
                convertedHandles->Handles[i].UniqueProcessId = ProcessId;
                convertedHandles->Handles[i].HandleValue = handle->HandleValue;
                convertedHandles->Handles[i].GrantedAccess = handle->GrantedAccess;
                convertedHandles->Handles[i].CreatorBackTraceIndex = 0;
                convertedHandles->Handles[i].ObjectTypeIndex = (USHORT)handle->ObjectTypeIndex;
                convertedHandles->Handles[i].HandleAttributes = handle->HandleAttributes;
            }

            PhFreePage(handles);

            *Handles = convertedHandles;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PSYSTEM_HANDLE_INFORMATION_EX handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i, numberOfHandles = 0;
        ULONG firstIndex = 0, lastIndex = 0;

        if (NT_SUCCESS(status = PhEnumHandlesEx(&handles)))
        {
            for (i = 0; i < handles->NumberOfHandles; i++)
            {
                PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = &handles->Handles[i];

                if (handle->UniqueProcessId == ProcessId)
                {
                    if (numberOfHandles == 0)
                        firstIndex = i;
                    lastIndex = i;
                    numberOfHandles++;
                }
            }

            if (numberOfHandles)
            {
                convertedHandles = PhAllocate(UFIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[numberOfHandles]));
                convertedHandles->NumberOfHandles = numberOfHandles;

                if (lastIndex == firstIndex + numberOfHandles - 1) // consecutive
                {
                    memcpy(
                        &convertedHandles->Handles[0],
                        &handles->Handles[firstIndex],
                        numberOfHandles * sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX)
                        );

                    *Handles = convertedHandles;
                }
                else
                {
                    ULONG j = 0;

                    for (i = firstIndex; i <= lastIndex; i++)
                    {
                        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = &handles->Handles[i];

                        if (handle->UniqueProcessId == ProcessId)
                        {
                            memcpy(&convertedHandles->Handles[j++], handle, sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX));
                        }
                    }

                    *Handles = convertedHandles;
                }
            }
            else
            {
                status = STATUS_NOT_FOUND;
            }

            PhFree(handles);
        }

        return status;
    }

    return status;
}

/**
 * Enumerates all pagefiles.
 *
 * \param Pagefiles A variable which receives a pointer to a buffer containing information about all
 * active pagefiles. You must free the structure using PhFree() when you no longer need it.
 *
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 */
NTSTATUS PhEnumPagefiles(
    _Out_ PVOID *Pagefiles
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x200;

    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(
        SystemPageFileInformation,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Pagefiles = buffer;

    return status;
}

NTSTATUS PhEnumPagefilesEx(
    _Out_ PVOID *Pagefiles
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x200;

    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(
        SystemPageFileInformationEx,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Pagefiles = buffer;

    return status;
}

/**
 * Enumerates pool tag information.
 *
 * \param Buffer A pointer to a buffer that will receive the pool tag information.
 * \return The status of the operation.
 */
NTSTATUS PhEnumPoolTagInformation(
    _Out_ PVOID* Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemPoolTagInformation,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemPoolTagInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
}

/**
 * Enumerates information about the big pool allocations in the system.
 *
 * \param Buffer A pointer to a variable that receives the buffer containing the big pool information.
 *
 * \return The status of the operation.
 */
NTSTATUS PhEnumBigPoolInformation(
    _Out_ PVOID* Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemBigPoolInformation,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        buffer = PhReAllocate(buffer, bufferSize);

        status = NtQuerySystemInformation(
            SystemBigPoolInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
}

static BOOLEAN NTAPI PhIsContainerEnumCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ PVOID Context
    )
{
    static CONST PH_STRINGREF typeName = PH_STRINGREF_INIT(L"Job");
    HANDLE objectHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(Name, &objectName))
        return TRUE;
    if (!PhEqualStringRef(TypeName, &typeName, FALSE))
        return TRUE;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    if (NT_SUCCESS(NtOpenJobObject(&objectHandle, JOB_OBJECT_QUERY, &objectAttributes)))
    {
        NtClose(objectHandle);
    }

    return TRUE;
}

/**
 * Determines if a process is managed.
 *
 * \param ProcessId The ID of the process.
 * \param ProcessHandle A handle to the process.
 * \param IsContainer A variable which receives a boolean indicating whether the process is managed.
 */
NTSTATUS PhGetProcessIsContainer(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _Out_opt_ PBOOLEAN IsContainer
    )
{
    static CONST PH_STRINGREF directoryName = PH_STRINGREF_INIT(L"\\");
    NTSTATUS status;
    HANDLE directoryHandle;

    status = PhOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        NULL,
        &directoryName
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhEnumDirectoryObjects(
        directoryHandle,
        PhIsContainerEnumCallback,
        NULL
        );

    NtClose(directoryHandle);

    return status;
}

/**
 * Determines if a process is managed.
 *
 * \param ProcessId The ID of the process.
 * \param IsDotNet A variable which receives a boolean indicating whether the process is managed.
 */
NTSTATUS PhGetProcessIsDotNet(
    _In_ HANDLE ProcessId,
    _Out_ PBOOLEAN IsDotNet
    )
{
    return PhGetProcessIsDotNetEx(ProcessId, NULL, 0, IsDotNet, NULL);
}

_Function_class_(PH_ENUM_PROCESS_MODULES_CALLBACK)
BOOLEAN NTAPI PhpIsDotNetEnumProcessModulesCallback(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_ PVOID Context
    )
{
    static CONST PH_STRINGREF clrString = PH_STRINGREF_INIT(L"clr.dll");
    static CONST PH_STRINGREF clrcoreString = PH_STRINGREF_INIT(L"coreclr.dll");
    static CONST PH_STRINGREF mscorwksString = PH_STRINGREF_INIT(L"mscorwks.dll");
    static CONST PH_STRINGREF mscorsvrString = PH_STRINGREF_INIT(L"mscorsvr.dll");
    static CONST PH_STRINGREF mscorlibString = PH_STRINGREF_INIT(L"mscorlib.dll");
    static CONST PH_STRINGREF mscorlibNiString = PH_STRINGREF_INIT(L"mscorlib.ni.dll");
    static CONST PH_STRINGREF frameworkString = PH_STRINGREF_INIT(L"\\Microsoft.NET\\Framework\\");
    static CONST PH_STRINGREF framework64String = PH_STRINGREF_INIT(L"\\Microsoft.NET\\Framework64\\");
    PH_STRINGREF baseDllName;

    PhUnicodeStringToStringRef(&Module->BaseDllName, &baseDllName);

    if (
        PhEqualStringRef(&baseDllName, &clrString, TRUE) ||
        PhEqualStringRef(&baseDllName, &mscorwksString, TRUE) ||
        PhEqualStringRef(&baseDllName, &mscorsvrString, TRUE)
        )
    {
        PH_STRINGREF fileName;
        PH_STRINGREF systemRoot;
        PCPH_STRINGREF frameworkPart;

#ifdef _WIN64
        if (*(PULONG)Context & PH_CLR_PROCESS_IS_WOW64)
        {
#endif
            frameworkPart = &frameworkString;
#ifdef _WIN64
        }
        else
        {
            frameworkPart = &framework64String;
        }
#endif

        PhUnicodeStringToStringRef(&Module->FullDllName, &fileName);
        PhGetSystemRoot(&systemRoot);

        if (PhStartsWithStringRef(&fileName, &systemRoot, TRUE))
        {
            fileName.Buffer = PTR_ADD_OFFSET(fileName.Buffer, systemRoot.Length);
            fileName.Length -= systemRoot.Length;

            if (PhStartsWithStringRef(&fileName, frameworkPart, TRUE))
            {
                fileName.Buffer = PTR_ADD_OFFSET(fileName.Buffer, frameworkPart->Length);
                fileName.Length -= frameworkPart->Length;

                if (fileName.Length >= 4 * sizeof(WCHAR)) // vx.x
                {
                    if (fileName.Buffer[1] == L'1')
                    {
                        if (fileName.Buffer[3] == L'0')
                            *(PULONG)Context |= PH_CLR_VERSION_1_0;
                        else if (fileName.Buffer[3] == L'1')
                            *(PULONG)Context |= PH_CLR_VERSION_1_1;
                    }
                    else if (fileName.Buffer[1] == L'2')
                    {
                        *(PULONG)Context |= PH_CLR_VERSION_2_0;
                    }
                    else if (fileName.Buffer[1] >= L'4' && fileName.Buffer[1] <= L'9')
                    {
                        *(PULONG)Context |= PH_CLR_VERSION_4_ABOVE;
                    }
                }
            }
        }
    }
    else if (
        PhEqualStringRef(&baseDllName, &mscorlibString, TRUE) ||
        PhEqualStringRef(&baseDllName, &mscorlibNiString, TRUE)
        )
    {
        *(PULONG)Context |= PH_CLR_MSCORLIB_PRESENT;
    }
    else if (PhEqualStringRef(&baseDllName, &clrcoreString, TRUE))
    {
        *(PULONG)Context |= PH_CLR_CORELIB_PRESENT;
    }

    return TRUE;
}

typedef struct _PHP_PIPE_NAME_HASH
{
    ULONG NameHash;
    ULONG Found;
} PHP_PIPE_NAME_HASH, *PPHP_PIPE_NAME_HASH;

_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN NTAPI PhpDotNetCorePipeHashCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PPHP_PIPE_NAME_HASH context = Context;
    PH_STRINGREF objectName;

    objectName.Length = Information->FileNameLength;
    objectName.Buffer = Information->FileName;

    if (PhHashStringRefEx(&objectName, FALSE, PH_STRING_HASH_X65599) == context->NameHash)
    {
        context->Found = TRUE;
        return FALSE;
    }

    return TRUE;
}

/**
 * Determines if a process is managed.
 *
 * \param ProcessId The ID of the process.
 * \param ProcessHandle An optional handle to the process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param InFlags A combination of flags.
 * \li \c PH_CLR_USE_SECTION_CHECK Checks for the existence of related section objects to determine
 * whether the process is managed.
 * \li \c PH_CLR_NO_WOW64_CHECK Instead of a separate query, uses the presence of the
 * \c PH_CLR_KNOWN_IS_WOW64 flag to determine whether the process is running under WOW64.
 * \li \c PH_CLR_KNOWN_IS_WOW64 When \c PH_CLR_NO_WOW64_CHECK is specified, indicates that the
 * process is managed.
 * \param IsDotNet A variable which receives a boolean indicating whether the process is managed.
 * \param Flags A variable which receives additional flags.
 */
NTSTATUS PhGetProcessIsDotNetEx(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ ULONG InFlags,
    _Out_opt_ PBOOLEAN IsDotNet,
    _Out_opt_ PULONG Flags
    )
{
    if (InFlags & PH_CLR_USE_SECTION_CHECK)
    {
        NTSTATUS status;
        HANDLE sectionHandle;
        SIZE_T returnLength;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING objectName;
        PH_STRINGREF objectNameStringRef;
        PH_FORMAT format[2];
        WCHAR formatBuffer[0x80];

        // Most .NET processes have a handle open to a section named
        // \BaseNamedObjects\Cor_Private_IPCBlock(_v4)_<ProcessId>. This is the same object used by
        // the ICorPublish::GetProcess function. Instead of calling that function, we simply check
        // for the existence of that section object. This means:
        // * Better performance.
        // * No need for admin rights to get .NET status of processes owned by other users.

        // Version 4 section object

        PhInitFormatS(&format[0], L"\\BaseNamedObjects\\Cor_Private_IPCBlock_v4_");
        PhInitFormatU(&format[1], HandleToUlong(ProcessId));

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            objectNameStringRef.Length = returnLength - sizeof(UNICODE_NULL);
            objectNameStringRef.Buffer = formatBuffer;

            PhStringRefToUnicodeString(&objectNameStringRef, &objectName);
        }
        else
        {
            RtlInitEmptyUnicodeString(&objectName, NULL, 0);
        }

        InitializeObjectAttributes(
            &objectAttributes,
            &objectName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        status = NtOpenSection(
            &sectionHandle,
            SECTION_QUERY,
            &objectAttributes
            );

        if (NT_SUCCESS(status) || status == STATUS_ACCESS_DENIED)
        {
            if (NT_SUCCESS(status))
                NtClose(sectionHandle);

            if (IsDotNet)
                *IsDotNet = TRUE;

            if (Flags)
                *Flags = PH_CLR_VERSION_4_ABOVE;

            return STATUS_SUCCESS;
        }

        // Version 2 section object

        PhInitFormatS(&format[0], L"\\BaseNamedObjects\\Cor_Private_IPCBlock_");
        PhInitFormatU(&format[1], HandleToUlong(ProcessId));

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            objectNameStringRef.Length = returnLength - sizeof(UNICODE_NULL);
            objectNameStringRef.Buffer = formatBuffer;

            PhStringRefToUnicodeString(&objectNameStringRef, &objectName);
        }
        else
        {
            RtlInitEmptyUnicodeString(&objectName, NULL, 0);
        }

        InitializeObjectAttributes(
            &objectAttributes,
            &objectName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );
        status = NtOpenSection(
            &sectionHandle,
            SECTION_QUERY,
            &objectAttributes
            );

        if (NT_SUCCESS(status) || status == STATUS_ACCESS_DENIED)
        {
            if (NT_SUCCESS(status))
                NtClose(sectionHandle);

            if (IsDotNet)
                *IsDotNet = TRUE;

            if (Flags)
                *Flags = PH_CLR_VERSION_2_0;

            return STATUS_SUCCESS;
        }

        // .NET Core 3.0 and above objects

        PhInitFormatS(&format[0], L"dotnet-diagnostic-");
        PhInitFormatU(&format[1], HandleToUlong(ProcessId));

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            PHP_PIPE_NAME_HASH context;

            objectNameStringRef.Length = returnLength - sizeof(UNICODE_NULL);
            objectNameStringRef.Buffer = formatBuffer;
            context.NameHash = PhHashStringRefEx(&objectNameStringRef, FALSE, PH_STRING_HASH_X65599);
            context.Found = FALSE;

            status = PhEnumDirectoryNamedPipe(
                &objectNameStringRef,
                PhpDotNetCorePipeHashCallback,
                &context
                );

            if (NT_SUCCESS(status))
            {
                if (!context.Found)
                {
                    status = STATUS_OBJECT_NAME_NOT_FOUND;
                }
            }

            // NOTE: NtQueryAttributesFile and other query functions connect to the pipe and should be avoided. (dmex)
            //
            //FILE_BASIC_INFORMATION fileInfo;
            //
            //objectNameSr.Length = returnLength - sizeof(UNICODE_NULL);
            //objectNameSr.Buffer = formatBuffer;
            //
            //PhStringRefToUnicodeString(&objectNameSr, &objectName);
            //InitializeObjectAttributes(
            //    &objectAttributes,
            //    &objectName,
            //    OBJ_CASE_INSENSITIVE,
            //    NULL,
            //    NULL
            //    );
            //
            //status = NtQueryAttributesFile(&objectAttributes, &fileInfo)
        }

        if (NT_SUCCESS(status))
        {
            if (IsDotNet)
                *IsDotNet = TRUE;
            if (Flags)
                *Flags = PH_CLR_VERSION_4_ABOVE | PH_CLR_CORE_3_0_ABOVE;

            return STATUS_SUCCESS;
        }

        return status;
    }
    else
    {
        NTSTATUS status;
        HANDLE processHandle = NULL;
        ULONG flags = 0;
#ifdef _WIN64
        BOOLEAN isWow64;
#endif

        if (!ProcessHandle)
        {
            if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, ProcessId)))
                return status;

            ProcessHandle = processHandle;
        }

#ifdef _WIN64
        if (InFlags & PH_CLR_NO_WOW64_CHECK)
        {
            isWow64 = !!(InFlags & PH_CLR_KNOWN_IS_WOW64);
        }
        else
        {
            isWow64 = FALSE;
            PhGetProcessIsWow64(ProcessHandle, &isWow64);
        }

        if (isWow64)
        {
            flags |= PH_CLR_PROCESS_IS_WOW64;
            status = PhEnumProcessModules32(ProcessHandle, PhpIsDotNetEnumProcessModulesCallback, &flags);
        }
        else
        {
#endif
            status = PhEnumProcessModules(ProcessHandle, PhpIsDotNetEnumProcessModulesCallback, &flags);
#ifdef _WIN64
        }
#endif

        if (processHandle)
            NtClose(processHandle);

        if (IsDotNet)
            *IsDotNet = (flags & PH_CLR_VERSION_MASK) && (flags & (PH_CLR_MSCORLIB_PRESENT | PH_CLR_CORELIB_PRESENT));

        if (Flags)
            *Flags = flags;

        return status;
    }
}

/*
 * Opens a directory object.
 *
 * \param DirectoryHandle A variable which receives a handle to the directory object.
 * \param DesiredAccess The desired access to the directory object.
 * \param RootDirectory A handle to the root directory of the object.
 * \param ObjectName The name of the directory object.
 *
 * \return Returns the status of the operation.
 */
NTSTATUS PhOpenDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
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

    status = NtOpenDirectoryObject(
        DirectoryHandle,
        DesiredAccess,
        &objectAttributes
        );

    return status;
}

/**
 * Enumerates the objects in a directory object.
 *
 * \param DirectoryHandle A handle to a directory. The handle must have DIRECTORY_QUERY access.
 * \param Callback A callback function which is executed for each object.
 * \param Context A user-defined value to pass to the callback function.
 */
NTSTATUS PhEnumDirectoryObjects(
    _In_ HANDLE DirectoryHandle,
    _In_ PPH_ENUM_DIRECTORY_OBJECTS Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    ULONG context = 0;
    BOOLEAN firstTime = TRUE;
    ULONG bufferSize;
    POBJECT_DIRECTORY_INFORMATION buffer;
    ULONG i;
    BOOLEAN result;

    bufferSize = 0x200;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        // Get a batch of entries.

        while ((status = NtQueryDirectoryObject(
            DirectoryHandle,
            buffer,
            bufferSize,
            FALSE,
            firstTime,
            &context,
            NULL
            )) == STATUS_MORE_ENTRIES)
        {
            // Check if we have at least one entry. If not, we'll double the buffer size and try
            // again.
            if (buffer[0].Name.Buffer)
                break;

            // Make sure we don't use too much memory.
            if (bufferSize > PH_LARGE_BUFFER_SIZE)
            {
                PhFree(buffer);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            PhFree(buffer);
            bufferSize *= 2;
            buffer = PhAllocate(bufferSize);
        }

        if (!NT_SUCCESS(status))
        {
            PhFree(buffer);
            return status;
        }

        // Read the batch and execute the callback function for each object.

        i = 0;
        result = TRUE;

        while (TRUE)
        {
            POBJECT_DIRECTORY_INFORMATION info;
            PH_STRINGREF name;
            PH_STRINGREF typeName;

            info = &buffer[i];

            if (!info->Name.Buffer)
                break;

            PhUnicodeStringToStringRef(&info->Name, &name);
            PhUnicodeStringToStringRef(&info->TypeName, &typeName);

            result = Callback(DirectoryHandle, &name, &typeName, Context);

            if (!result)
                break;

            i++;
        }

        if (!result)
            break;

        if (status != STATUS_MORE_ENTRIES)
            break;

        firstTime = FALSE;
    }

    PhFree(buffer);

    return STATUS_SUCCESS;
}

NTSTATUS PhCreateSymbolicLinkObject(
    _Out_ PHANDLE LinkHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCPH_STRINGREF FileName,
    _In_ PCPH_STRINGREF LinkName
    )
{
    NTSTATUS status;
    HANDLE linkHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    UNICODE_STRING objectTarget;

    if (!PhStringRefToUnicodeString(FileName, &objectName))
        return STATUS_NAME_TOO_LONG;
    if (!PhStringRefToUnicodeString(LinkName, &objectTarget))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateSymbolicLinkObject(
        &linkHandle,
        DesiredAccess,
        &objectAttributes,
        &objectTarget
        );

    if (NT_SUCCESS(status))
    {
        *LinkHandle = linkHandle;
    }

    return status;
}

NTSTATUS PhQuerySymbolicLinkObject(
    _Out_ PPH_STRING* LinkTarget,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName
    )
{
    NTSTATUS status;
    HANDLE linkHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    UNICODE_STRING targetName;
    WCHAR targetNameBuffer[DOS_MAX_PATH_LENGTH];

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenSymbolicLinkObject(
        &linkHandle,
        SYMBOLIC_LINK_QUERY,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        return status;

    RtlInitEmptyUnicodeString(&targetName, targetNameBuffer, sizeof(targetNameBuffer));

    status = NtQuerySymbolicLinkObject(
        linkHandle,
        &targetName,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *LinkTarget = PhCreateStringFromUnicodeString(&targetName);
    }

    NtClose(linkHandle);

    return status;
}

/**
 * Initializes the device prefixes module.
 */
VOID PhpInitializeDevicePrefixes(
    VOID
    )
{
    ULONG i;
    PWCHAR buffer;

    // Allocate one buffer for all 26 prefixes to reduce overhead.
    buffer = PhAllocate(PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR) * 26);

    for (i = 0; i < 26; i++)
    {
        PhDevicePrefixes[i].Length = 0;
        PhDevicePrefixes[i].MaximumLength = PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR);
        PhDevicePrefixes[i].Buffer = buffer;
        buffer = PTR_ADD_OFFSET(buffer, PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR));
    }
}

VOID PhUpdateMupDevicePrefixes(
    VOID
    )
{
    static CONST PH_STRINGREF orderKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Control\\NetworkProvider\\Order");
    static CONST PH_STRINGREF servicesStringPart = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");
    static CONST PH_STRINGREF networkProviderStringPart = PH_STRINGREF_INIT(L"\\NetworkProvider");

    HANDLE orderKeyHandle;
    PPH_STRING providerOrder = NULL;
    ULONG i;
    PH_STRINGREF remainingPart;
    PH_STRINGREF part;

    // The provider names are stored in the ProviderOrder value in this key:
    // HKLM\System\CurrentControlSet\Control\NetworkProvider\Order
    // Each name can then be looked up, its device name in the DeviceName value in:
    // HKLM\System\CurrentControlSet\Services\<ProviderName>\NetworkProvider

    // Note that we assume the providers only claim their device name. Some providers such as DFS
    // claim an extra part, and are not resolved correctly here.

    if (NT_SUCCESS(PhOpenKey(
        &orderKeyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &orderKeyName,
        0
        )))
    {
        providerOrder = PhQueryRegistryStringZ(orderKeyHandle, L"ProviderOrder");
        NtClose(orderKeyHandle);
    }

    if (!providerOrder)
        return;

    PhAcquireQueuedLockExclusive(&PhDeviceMupPrefixesLock);

    for (i = 0; i < PhDeviceMupPrefixesCount; i++)
    {
        PhDereferenceObject(PhDeviceMupPrefixes[i]);
        PhDeviceMupPrefixes[i] = NULL;
    }

    PhDeviceMupPrefixesCount = 0;

    PhDeviceMupPrefixes[PhDeviceMupPrefixesCount++] = PhCreateString(L"\\Device\\Mup");

    // DFS claims an extra part of file names, which we don't handle.
    // PhDeviceMupPrefixes[PhDeviceMupPrefixesCount++] = PhCreateString(L"\\Device\\DfsClient");

    remainingPart = providerOrder->sr;

    while (remainingPart.Length != 0)
    {
        PPH_STRING serviceKeyName;
        HANDLE networkProviderKeyHandle;
        PPH_STRING deviceName;

        if (PhDeviceMupPrefixesCount == PH_DEVICE_MUP_PREFIX_MAX_COUNT)
            break;

        PhSplitStringRefAtChar(&remainingPart, L',', &part, &remainingPart);

        if (part.Length != 0)
        {
            serviceKeyName = PhConcatStringRef3(&servicesStringPart, &part, &networkProviderStringPart);

            if (NT_SUCCESS(PhOpenKey(
                &networkProviderKeyHandle,
                KEY_READ,
                PH_KEY_LOCAL_MACHINE,
                &serviceKeyName->sr,
                0
                )))
            {
                if (deviceName = PhQueryRegistryStringZ(networkProviderKeyHandle, L"DeviceName"))
                {
                    PhDeviceMupPrefixes[PhDeviceMupPrefixesCount] = deviceName;
                    PhDeviceMupPrefixesCount++;
                }

                NtClose(networkProviderKeyHandle);
            }

            PhDereferenceObject(serviceKeyName);
        }
    }

    PhReleaseQueuedLockExclusive(&PhDeviceMupPrefixesLock);

    PhDereferenceObject(providerOrder);
}

/**
 * Updates the DOS device names array.
 */
VOID PhUpdateDosDevicePrefixes(
    VOID
    )
{
    WCHAR deviceNameBuffer[7] = L"\\??\\ :";
    ULONG deviceMap = 0;

    PhGetProcessDeviceMap(NtCurrentProcess(), &deviceMap);

    PhAcquireQueuedLockExclusive(&PhDevicePrefixesLock);

    for (ULONG i = 0; i < 0x1A; i++)
    {
        HANDLE linkHandle;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING deviceName;

        if (deviceMap)
        {
            if (!(deviceMap & (0x1 << i)))
            {
                PhDevicePrefixes[i].Length = 0;
                continue;
            }
        }

        deviceNameBuffer[4] = (WCHAR)('A' + i);
        deviceName.Buffer = deviceNameBuffer;
        deviceName.Length = sizeof(deviceNameBuffer) - sizeof(UNICODE_NULL);
        deviceName.MaximumLength = deviceName.Length + sizeof(UNICODE_NULL);

        InitializeObjectAttributes(
            &objectAttributes,
            &deviceName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        if (NT_SUCCESS(NtOpenSymbolicLinkObject(
            &linkHandle,
            SYMBOLIC_LINK_QUERY,
            &objectAttributes
            )))
        {
            if (!NT_SUCCESS(NtQuerySymbolicLinkObject(
                linkHandle,
                &PhDevicePrefixes[i],
                NULL
                )))
            {
                PhDevicePrefixes[i].Length = 0;
            }

            NtClose(linkHandle);
        }
        else
        {
            PhDevicePrefixes[i].Length = 0;
        }
    }

    PhReleaseQueuedLockExclusive(&PhDevicePrefixesLock);
}

// rev from FindFirstVolumeW (dmex)
/**
 * \brief Retrieves the mount points of volumes.
 *
 * \param DeviceHandle A handle to the MountPointManager.
 * \param MountPoints An array of mounts.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetVolumeMountPoints(
    _In_ HANDLE DeviceHandle,
    _Out_ PMOUNTMGR_MOUNT_POINTS* MountPoints
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    MOUNTMGR_MOUNT_POINT inputBuffer = { 0 };
    PMOUNTMGR_MOUNT_POINTS outputBuffer;
    ULONG inputBufferLength = sizeof(inputBuffer);
    ULONG outputBufferLength;
    ULONG attempts = 16;

    outputBufferLength = 0x800;
    outputBuffer = PhAllocate(outputBufferLength);

    do
    {
        status = NtDeviceIoControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            IOCTL_MOUNTMGR_QUERY_POINTS,
            &inputBuffer,
            inputBufferLength,
            outputBuffer,
            outputBufferLength
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            outputBufferLength = outputBuffer->Size;
            PhFree(outputBuffer);
            outputBuffer = PhAllocate(outputBufferLength);
        }
        else
        {
            PhFree(outputBuffer);
            return status;
        }
    } while (--attempts);

    if (NT_SUCCESS(status))
    {
        *MountPoints = outputBuffer;
    }
    else
    {
        PhFree(outputBuffer);
    }

    return status;
}

// rev from GetVolumePathNamesForVolumeNameW (dmex)
/**
 * \brief Retrieves a list of drive letters and mounted folder paths for the specified volume.
 *
 * \param DeviceHandle A handle to the MountPointManager.
 * \param VolumeName A volume GUID path for the volume.
 * \param VolumePathNames A pointer to a buffer that receives the list of drive letters and mounted folder paths.
 * \a The list is an array of null-terminated strings terminated by an additional NULL character.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetVolumePathNamesForVolumeName(
    _In_ HANDLE DeviceHandle,
    _In_ PCPH_STRINGREF VolumeName,
    _Out_ PMOUNTMGR_VOLUME_PATHS* VolumePathNames
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PMOUNTMGR_TARGET_NAME inputBuffer;
    PMOUNTMGR_VOLUME_PATHS outputBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    ULONG attempts = 16;

    inputBufferLength = UFIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName[VolumeName->Length]) + sizeof(UNICODE_NULL);
    inputBuffer = PhAllocate(inputBufferLength); // Volume{guid}, CM_Get_Device_Interface_List, SymbolicLinks, [??]
    inputBuffer->DeviceNameLength = (USHORT)VolumeName->Length;
    RtlCopyMemory(inputBuffer->DeviceName, VolumeName->Buffer, VolumeName->Length);

    outputBufferLength = UFIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz[DOS_MAX_PATH_LENGTH]) + sizeof(UNICODE_NULL);
    outputBuffer = PhAllocate(outputBufferLength);

    do
    {
        status = NtDeviceIoControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS,
            inputBuffer,
            inputBufferLength,
            outputBuffer,
            outputBufferLength
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            outputBufferLength = (outputBuffer->MultiSzLength * sizeof(WCHAR)) + sizeof(UNICODE_NULL);
            PhFree(outputBuffer);
            outputBuffer = PhAllocate(outputBufferLength);
        }
        else
        {
            PhFree(inputBuffer);
            PhFree(outputBuffer);
            return status;
        }
    } while (--attempts);

    if (NT_SUCCESS(status))
    {
        *VolumePathNames = outputBuffer;
    }
    else
    {
        PhFree(outputBuffer);
    }

    PhFree(inputBuffer);

    return status;
}

/**
 * Flush file caches on all volumes.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhFlushVolumeCache(
    VOID
    )
{
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    PMOUNTMGR_MOUNT_POINTS objectMountPoints;

    RtlInitUnicodeString(&objectName, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &deviceHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetVolumeMountPoints(
        deviceHandle,
        &objectMountPoints
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    for (ULONG i = 0; i < objectMountPoints->NumberOfMountPoints; i++)
    {
        PMOUNTMGR_MOUNT_POINT mountPoint = &objectMountPoints->MountPoints[i];
        objectName.Length = mountPoint->SymbolicLinkNameLength;
        objectName.MaximumLength = mountPoint->SymbolicLinkNameLength + sizeof(UNICODE_NULL);
        objectName.Buffer = PTR_ADD_OFFSET(objectMountPoints, mountPoint->SymbolicLinkNameOffset);

        if (MOUNTMGR_IS_VOLUME_NAME(&objectName)) // \\??\\Volume{1111-2222}
        {
            HANDLE volumeHandle;

            InitializeObjectAttributes(
                &objectAttributes,
                &objectName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

            status = NtCreateFile(
                &volumeHandle,
                FILE_WRITE_DATA | SYNCHRONIZE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

            if (NT_SUCCESS(status))
            {
                //if (WindowsVersion >= WINDOWS_8)
                //{
                //    status = NtFlushBuffersFileEx(volumeHandle, 0, 0, 0, &ioStatusBlock);
                //}
                //else
                {
                    status = NtFlushBuffersFile(volumeHandle, &ioStatusBlock);
                }

                NtClose(volumeHandle);
            }
        }
    }

    PhFree(objectMountPoints);

CleanupExit:
    NtClose(deviceHandle);

    return status;
}

NTSTATUS PhUpdateDosDeviceMountPrefixes(
    VOID
    )
{
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    PMOUNTMGR_MOUNT_POINTS deviceMountPoints;

    RtlInitUnicodeString(&objectName, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &deviceHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetVolumeMountPoints(
        deviceHandle,
        &deviceMountPoints
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhDevicePrefixes); i++)
    {
        PhDevicePrefixes[i].Length = 0;
    }

    for (ULONG i = 0; i < deviceMountPoints->NumberOfMountPoints; i++)
    {
        PMOUNTMGR_MOUNT_POINT entry = &deviceMountPoints->MountPoints[i];
        UNICODE_STRING linkName =
        {
            entry->SymbolicLinkNameLength,
            entry->SymbolicLinkNameLength + sizeof(UNICODE_NULL),
            PTR_ADD_OFFSET(deviceMountPoints, entry->SymbolicLinkNameOffset)
        };
        UNICODE_STRING deviceName =
        {
            entry->DeviceNameLength,
            entry->DeviceNameLength + sizeof(UNICODE_NULL),
            PTR_ADD_OFFSET(deviceMountPoints, entry->DeviceNameOffset)
        };

        if (MOUNTMGR_IS_DRIVE_LETTER(&linkName)) // \\DosDevices\\C:
        {
            USHORT index = (USHORT)(linkName.Buffer[12] - L'A');

            if (index >= RTL_NUMBER_OF(PhDevicePrefixes))
                continue;
            if (deviceName.Length >= PhDevicePrefixes[index].MaximumLength - sizeof(UNICODE_NULL))
                continue;

            PhDevicePrefixes[index].Length = deviceName.Length;
            memcpy_s(
                PhDevicePrefixes[index].Buffer,
                PhDevicePrefixes[index].MaximumLength,
                deviceName.Buffer,
                deviceName.Length
                );
        }

        //if (MOUNTMGR_IS_VOLUME_NAME(&linkName)) // \\??\\Volume{1111-2222}
        //{
        //    PH_STRINGREF volumeLinkName;
        //    PMOUNTMGR_VOLUME_PATHS volumePaths;
        //
        //    PhUnicodeStringToStringRef(&linkName, &volumeLinkName);
        //
        //    if (NT_SUCCESS(PhGetVolumePathNamesForVolumeName(deviceHandle, &volumeLinkName, &volumePaths)))
        //    {
        //        for (PWSTR path = volumePaths->MultiSz; *path; path += PhCountStringZ(path) + 1)
        //        {
        //            dprintf("%S\n", path); // C:\\Mounted\\Folders
        //        }
        //    }
        //}
    }

    PhFree(deviceMountPoints);

CleanupExit:
    NtClose(deviceHandle);

    return status;
}

/**
 * Resolves a NT path into a Win32 path.
 *
 * \param Name A string containing the path to resolve.
 *
 * \return A pointer to a string containing the Win32 path. You must free the string using
 * PhDereferenceObject() when you no longer need it.
 */
PPH_STRING PhResolveDevicePrefix(
    _In_ PCPH_STRINGREF Name
    )
{
    ULONG i;
    PPH_STRING newName = NULL;

    if (PhBeginInitOnce(&PhDevicePrefixesInitOnce))
    {
        PhpInitializeDevicePrefixes();
        PhUpdateDosDevicePrefixes();
        PhUpdateMupDevicePrefixes();

        //PhUpdateDosDeviceMountPrefixes();

        PhEndInitOnce(&PhDevicePrefixesInitOnce);
    }

    PhAcquireQueuedLockShared(&PhDevicePrefixesLock);

    // Go through the DOS devices and try to find a matching prefix.
    for (i = 0; i < 26; i++)
    {
        BOOLEAN isPrefix = FALSE;
        PH_STRINGREF prefix;

        PhUnicodeStringToStringRef(&PhDevicePrefixes[i], &prefix);

        if (prefix.Length != 0)
        {
            if (PhStartsWithStringRef(Name, &prefix, TRUE))
            {
                // To ensure we match the longest prefix, make sure the next character is a
                // backslash or the path is equal to the prefix.
                if (Name->Length == prefix.Length || Name->Buffer[prefix.Length / sizeof(WCHAR)] == OBJ_NAME_PATH_SEPARATOR)
                {
                    isPrefix = TRUE;
                }
            }
        }

        if (isPrefix)
        {
            // <letter>:path
            newName = PhCreateStringEx(NULL, 2 * sizeof(WCHAR) + Name->Length - prefix.Length);
            newName->Buffer[0] = (WCHAR)('A' + i);
            newName->Buffer[1] = L':';
            memcpy(
                &newName->Buffer[2],
                &Name->Buffer[prefix.Length / sizeof(WCHAR)],
                Name->Length - prefix.Length
                );

            break;
        }
    }

    PhReleaseQueuedLockShared(&PhDevicePrefixesLock);

    if (i == 26)
    {
        // Resolve network providers.

        PhAcquireQueuedLockShared(&PhDeviceMupPrefixesLock);

        for (i = 0; i < PhDeviceMupPrefixesCount; i++)
        {
            BOOLEAN isPrefix = FALSE;
            SIZE_T prefixLength;

            prefixLength = PhDeviceMupPrefixes[i]->Length;

            if (prefixLength != 0)
            {
                if (PhStartsWithStringRef(Name, &PhDeviceMupPrefixes[i]->sr, TRUE))
                {
                    // To ensure we match the longest prefix, make sure the next character is a
                    // backslash. Don't resolve if the name *is* the prefix. Otherwise, we will end
                    // up with a useless string like "\".
                    if (Name->Length != prefixLength && Name->Buffer[prefixLength / sizeof(WCHAR)] == OBJ_NAME_PATH_SEPARATOR)
                    {
                        isPrefix = TRUE;
                    }
                }
            }

            if (isPrefix)
            {
                // \path
                newName = PhCreateStringEx(NULL, 1 * sizeof(WCHAR) + Name->Length - prefixLength);
                newName->Buffer[0] = OBJ_NAME_PATH_SEPARATOR;
                memcpy(
                    &newName->Buffer[1],
                    &Name->Buffer[prefixLength / sizeof(WCHAR)],
                    Name->Length - prefixLength
                    );

                break;
            }
        }

        PhReleaseQueuedLockShared(&PhDeviceMupPrefixesLock);
    }

    if (newName)
        PhTrimToNullTerminatorString(newName);

    return newName;
}

/**
 * Converts a file name into Win32 format.
 *
 * \param FileName A string containing a file name.
 *
 * \return A pointer to a string containing the Win32 file name. You must free the string using
 * PhDereferenceObject() when you no longer need it.
 *
 * \remarks This function may convert NT object name paths to invalid ones. If the path to be
 * converted is not necessarily a file name, use PhResolveDevicePrefix().
 */
PPH_STRING PhGetFileName(
    _In_ PPH_STRING FileName
    )
{
    PPH_STRING newFileName;

    newFileName = FileName;

    // "\??\" refers to \GLOBAL??\. Just remove it.
    if (PhStartsWithString2(FileName, L"\\??\\", FALSE))
    {
        newFileName = PhCreateStringEx(NULL, FileName->Length - 4 * sizeof(WCHAR));
        memcpy(newFileName->Buffer, &FileName->Buffer[4], FileName->Length - 4 * sizeof(WCHAR));
    }
    // "\SystemRoot" means "C:\Windows".
    else if (PhStartsWithString2(FileName, L"\\SystemRoot", TRUE))
    {
        PH_STRINGREF systemRoot;

        PhGetSystemRoot(&systemRoot);
        newFileName = PhCreateStringEx(NULL, systemRoot.Length + FileName->Length - 11 * sizeof(WCHAR));
        memcpy(newFileName->Buffer, systemRoot.Buffer, systemRoot.Length);
        memcpy(PTR_ADD_OFFSET(newFileName->Buffer, systemRoot.Length), &FileName->Buffer[11], FileName->Length - 11 * sizeof(WCHAR));
    }
    // System32, SysWOW64, SysArm32, and SyChpe32 are all identical length, fixup is the same
    else if (
        // "System32\" means "C:\Windows\System32\".
        PhStartsWithString2(FileName, L"System32\\", TRUE)
#if _WIN64
        // "SysWOW64\" means "C:\Windows\SysWOW64\".
        || PhStartsWithString2(FileName, L"SysWOW64\\", TRUE)
#if _M_ARM64
        // "SysArm32\" means "C:\Windows\SysArm32\".
        || PhStartsWithString2(FileName, L"SysArm32\\", TRUE)
        // "SyChpe32\" means "C:\Windows\SyChpe32\".
        || PhStartsWithString2(FileName, L"SyChpe32\\", TRUE)
#endif
#endif
        )
    {
        PH_STRINGREF systemRoot;

        PhGetSystemRoot(&systemRoot);
        newFileName = PhCreateStringEx(NULL, systemRoot.Length + sizeof(UNICODE_NULL) + FileName->Length);
        memcpy(newFileName->Buffer, systemRoot.Buffer, systemRoot.Length);
        newFileName->Buffer[systemRoot.Length / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
        memcpy(PTR_ADD_OFFSET(newFileName->Buffer, systemRoot.Length + sizeof(UNICODE_NULL)), FileName->Buffer, FileName->Length);
    }
    else if (FileName->Length != 0 && FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        PPH_STRING resolvedName;

        resolvedName = PhResolveDevicePrefix(&FileName->sr);

        if (resolvedName)
        {
            newFileName = resolvedName;
        }
        else
        {
            // We didn't find a match.
            // If the file name starts with "\Windows", prepend the system drive.
            if (PhStartsWithString2(newFileName, L"\\Windows", TRUE))
            {
                PH_STRINGREF systemRoot;

                PhGetSystemRoot(&systemRoot);
                newFileName = PhCreateStringEx(NULL, FileName->Length + 2 * sizeof(WCHAR));
                newFileName->Buffer[0] = systemRoot.Buffer[0];
                newFileName->Buffer[1] = L':';
                memcpy(&newFileName->Buffer[2], FileName->Buffer, FileName->Length);
            }
            else
            {
                PhReferenceObject(newFileName);
            }
        }
    }
    else
    {
        // Just return the supplied file name. Note that we need to add a reference.
        PhReferenceObject(newFileName);
    }

    return newFileName;
}

PPH_STRING PhDosPathNameToNtPathName(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_STRING newName = NULL;
    PH_STRINGREF prefix;
    ULONG index;

    if (PhBeginInitOnce(&PhDevicePrefixesInitOnce))
    {
        PhpInitializeDevicePrefixes();
        PhUpdateDosDevicePrefixes();
        PhUpdateMupDevicePrefixes();

        PhEndInitOnce(&PhDevicePrefixesInitOnce);
    }

    if (PATH_IS_WIN32_DRIVE_PREFIX(Name))
    {
        index = (ULONG)(PhUpcaseUnicodeChar(Name->Buffer[0]) - L'A');

        if (index >= RTL_NUMBER_OF(PhDevicePrefixes))
            return NULL;

        PhAcquireQueuedLockShared(&PhDevicePrefixesLock);
        PhUnicodeStringToStringRef(&PhDevicePrefixes[index], &prefix);

        if (prefix.Length != 0)
        {
            // C:\\Name -> \\Device\\HardDiskVolumeX\\Name
            newName = PhCreateStringEx(NULL, prefix.Length + Name->Length - sizeof(WCHAR[2]));
            memcpy(
                newName->Buffer,
                prefix.Buffer,
                prefix.Length
                );
            memcpy(
                PTR_ADD_OFFSET(newName->Buffer, prefix.Length),
                PTR_ADD_OFFSET(Name->Buffer, sizeof(WCHAR[2])),
                Name->Length - sizeof(WCHAR[2])
                );
        }

        PhReleaseQueuedLockShared(&PhDevicePrefixesLock);
    }
    else if (PhStartsWithStringRef2(Name, L"\\SystemRoot", TRUE))
    {
        PhAcquireQueuedLockShared(&PhDevicePrefixesLock);
        PhUnicodeStringToStringRef(&PhDevicePrefixes[(ULONG)'C'-'A'], &prefix);

        if (prefix.Length != 0)
        {
            static CONST PH_STRINGREF systemRoot = PH_STRINGREF_INIT(L"\\Windows");

            // \\SystemRoot\\Name -> \\Device\\HardDiskVolumeX\\Windows\\Name
            newName = PhCreateStringEx(NULL, prefix.Length + Name->Length + systemRoot.Length - sizeof(L"SystemRoot"));
            memcpy(
                newName->Buffer,
                prefix.Buffer,
                prefix.Length
                );
            memcpy(
                PTR_ADD_OFFSET(newName->Buffer, prefix.Length),
                systemRoot.Buffer,
                systemRoot.Length
                );
            memcpy(
                PTR_ADD_OFFSET(newName->Buffer, prefix.Length + systemRoot.Length),
                PTR_ADD_OFFSET(Name->Buffer, sizeof(L"SystemRoot")),
                Name->Length - sizeof(L"SystemRoot")
                );
        }

        PhReleaseQueuedLockShared(&PhDevicePrefixesLock);
    }

    return newName;
}

NTSTATUS PhDosLongPathNameToNtPathNameWithStatus(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR* FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    )
{
    NTSTATUS status;

    if (
        WindowsVersion >= WINDOWS_10_RS1 && RtlAreLongPathsEnabled() &&
        RtlDosLongPathNameToNtPathName_U_WithStatus_Import()
        )
    {
        status = RtlDosLongPathNameToNtPathName_U_WithStatus_Import()(
            DosFileName,
            NtFileName,
            FilePart,
            RelativeName
            );
    }
    else
    {
        status = RtlDosPathNameToNtPathName_U_WithStatus(
            DosFileName,
            NtFileName,
            FilePart,
            RelativeName
            );
    }

    return status;
}

PPH_STRING PhGetNtPathRootPrefix(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_STRING pathDevicePrefix = NULL;
    PH_STRINGREF prefix;

    PhAcquireQueuedLockShared(&PhDevicePrefixesLock);

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhDevicePrefixes); i++)
    {
        PhUnicodeStringToStringRef(&PhDevicePrefixes[i], &prefix);

        if (prefix.Length && PhStartsWithStringRef(Name, &prefix, FALSE))
        {
            pathDevicePrefix = PhCreateString2(&prefix);
            break;
        }
    }

    PhReleaseQueuedLockShared(&PhDevicePrefixesLock);

    return pathDevicePrefix;
}

PPH_STRING PhGetExistingPathPrefix(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_STRING existingPathPrefix = NULL;
    PH_STRINGREF remainingPart;
    PH_STRINGREF directoryPart;
    PH_STRINGREF baseNamePart;

    if (PATH_IS_WIN32_DRIVE_PREFIX(Name))
    {
        assert(FALSE);
        return NULL;
    }

    if (PhDoesDirectoryExist(Name))
    {
        return PhCreateString2(Name);
    }

    remainingPart = *Name;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtLastChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &baseNamePart);

        if (directoryPart.Length != 0)
        {
            if (PhDoesDirectoryExist(&directoryPart))
            {
                existingPathPrefix = PhCreateString2(&directoryPart);
                break;
            }
        }

        remainingPart = directoryPart;
    }

    //if (PhEqualStringRef(&existingPathPrefix, PhGetNtPathRootPrefix(Name), FALSE))
    //    return NULL;

    return existingPathPrefix;
}

PPH_STRING PhGetExistingPathPrefixWin32(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_STRING existingPathPrefix = NULL;
    PH_STRINGREF remainingPart;
    PH_STRINGREF directoryPart;
    PH_STRINGREF baseNamePart;

    if (!PATH_IS_WIN32_DRIVE_PREFIX(Name))
    {
        assert(FALSE);
        return NULL;
    }

    if (PhDoesDirectoryExistWin32(PhGetStringRefZ(Name)))
    {
        return PhCreateString2(Name);
    }

    remainingPart = *Name;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtLastChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &baseNamePart);

        if (directoryPart.Length != 0)
        {
            existingPathPrefix = PhCreateString2(&directoryPart);

            if (PhDoesDirectoryExistWin32(PhGetString(existingPathPrefix)))
                break;

            PhClearReference(&existingPathPrefix);
        }

        remainingPart = directoryPart;
    }

    return existingPathPrefix;
}

// rev from GetLongPathNameW (dmex)
PPH_STRING PhGetLongPathName(
    _In_ PCPH_STRINGREF FileName
    )
{
    PPH_STRING longPathName = NULL;
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    PFILE_BOTH_DIR_INFORMATION directoryInfoBuffer;
    ULONG directoryInfoLength;
    PH_STRINGREF baseNamePart;
    UNICODE_STRING baseNameUs;

    status = PhOpenFile(
        &fileHandle,
        FileName,
        FILE_READ_DATA | FILE_LIST_DIRECTORY | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return NULL;

    if (!PhGetBasePath(FileName, NULL, &baseNamePart))
        goto CleanupExit;
    if (!PhStringRefToUnicodeString(&baseNamePart, &baseNameUs))
        goto CleanupExit;

    directoryInfoLength = PAGE_SIZE;
    directoryInfoBuffer = PhAllocate(directoryInfoLength);

    status = NtQueryDirectoryFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        directoryInfoBuffer,
        directoryInfoLength,
        FileBothDirectoryInformation,
        TRUE,
        &baseNameUs,
        FALSE
        );

    if (NT_SUCCESS(status))
    {
        longPathName = PhCreateStringEx(directoryInfoBuffer->FileName, directoryInfoBuffer->FileNameLength);
    }

    PhFree(directoryInfoBuffer);

CleanupExit:
    NtClose(fileHandle);

    return longPathName;
}

/**
 * Initializes usage of predefined keys.
 */
VOID PhpInitializePredefineKeys(
    VOID
    )
{
    static CONST UNICODE_STRING currentUserPrefix = RTL_CONSTANT_STRING(L"\\Registry\\User\\");
    NTSTATUS status;
    PH_TOKEN_USER tokenUser;
    UNICODE_STRING stringSid;
    WCHAR stringSidBuffer[SECURITY_MAX_SID_STRING_CHARACTERS];
    PUNICODE_STRING currentUserKeyName;

    // Get the string SID of the current user.

    if (NT_SUCCESS(status = PhGetTokenUser(PhGetOwnTokenAttributes().TokenHandle, &tokenUser)))
    {
        RtlInitEmptyUnicodeString(&stringSid, stringSidBuffer, sizeof(stringSidBuffer));

        if (PhEqualSid(tokenUser.User.Sid, (PSID)&PhSeLocalSystemSid))
        {
            status = RtlInitUnicodeStringEx(&stringSid, L".DEFAULT");
        }
        else
        {
            status = RtlConvertSidToUnicodeString(&stringSid, tokenUser.User.Sid, FALSE);
        }
    }

    // Construct the current user key name.

    if (NT_SUCCESS(status))
    {
        currentUserKeyName = &PhPredefineKeyNames[PH_KEY_CURRENT_USER_NUMBER];
        currentUserKeyName->Length = currentUserPrefix.Length + stringSid.Length;
        currentUserKeyName->MaximumLength = currentUserKeyName->Length + sizeof(UNICODE_NULL);
        currentUserKeyName->Buffer = PhAllocate(currentUserKeyName->MaximumLength);
        memcpy(currentUserKeyName->Buffer, currentUserPrefix.Buffer, currentUserPrefix.Length);
        memcpy(&currentUserKeyName->Buffer[currentUserPrefix.Length / sizeof(WCHAR)], stringSid.Buffer, stringSid.Length);
    }
}

/**
 * Initializes the attributes of a key object for creating/opening.
 *
 * \param RootDirectory A handle to a root key, or one of the predefined keys. See PhCreateKey() for
 * details.
 * \param ObjectName The path to the key.
 * \param Attributes Additional object flags.
 * \param ObjectAttributes The OBJECT_ATTRIBUTES structure to initialize.
 */
NTSTATUS PhpInitializeKeyObjectAttributes(
    _In_opt_ HANDLE RootDirectory,
    _In_ PUNICODE_STRING ObjectName,
    _In_ ULONG Attributes,
    _Out_ POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS status;
    ULONG predefineIndex;
    HANDLE predefineHandle;
    OBJECT_ATTRIBUTES predefineObjectAttributes;

    InitializeObjectAttributes(
        ObjectAttributes,
        ObjectName,
        Attributes | OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    if (RootDirectory && PH_KEY_IS_PREDEFINED(RootDirectory))
    {
        predefineIndex = PH_KEY_PREDEFINE_TO_NUMBER(RootDirectory);

        if (predefineIndex < PH_KEY_MAXIMUM_PREDEFINE)
        {
            if (PhBeginInitOnce(&PhPredefineKeyInitOnce))
            {
                PhpInitializePredefineKeys();
                PhEndInitOnce(&PhPredefineKeyInitOnce);
            }

            predefineHandle = InterlockedReadPointer(&PhPredefineKeyHandles[predefineIndex]);

            if (!predefineHandle)
            {
                // The predefined key has not been opened yet. Do so now.
                if (!PhPredefineKeyNames[predefineIndex].Buffer) // we may have failed in getting the current user key name
                    return STATUS_UNSUCCESSFUL;

                InitializeObjectAttributes(
                    &predefineObjectAttributes,
                    &PhPredefineKeyNames[predefineIndex],
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );

                status = NtOpenKey(
                    &predefineHandle,
                    KEY_READ,
                    &predefineObjectAttributes
                    );

                if (!NT_SUCCESS(status))
                    return status;

                HANDLE keyHandle = InterlockedCompareExchangePointer(
                    &PhPredefineKeyHandles[predefineIndex],
                    predefineHandle,
                    NULL
                    );

                if (keyHandle)
                {
                    // Someone else already opened the key and cached it.
                    NtClose(predefineHandle);
                    predefineHandle = keyHandle;
                }
            }

            ObjectAttributes->RootDirectory = predefineHandle;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Creates or opens a registry key.
 *
 * \param KeyHandle A variable which receives a handle to the key.
 * \param DesiredAccess The desired access to the key.
 * \param RootDirectory A handle to a root key, or one of the following predefined keys:
 * \li \c PH_KEY_LOCAL_MACHINE Represents \\Registry\\Machine.
 * \li \c PH_KEY_USERS Represents \\Registry\\User.
 * \li \c PH_KEY_CLASSES_ROOT Represents \\Registry\\Machine\\Software\\Classes.
 * \li \c PH_KEY_CURRENT_USER Represents \\Registry\\User\\[SID of current user].
 * \param ObjectName The path to the key.
 * \param Attributes Additional object flags.
 * \param CreateOptions The options to apply when creating or opening the key.
 * \param Disposition A variable which receives a value indicating whether a new key was created or
 * an existing key was opened:
 * \li \c REG_CREATED_NEW_KEY A new key was created.
 * \li \c REG_OPENED_EXISTING_KEY An existing key was opened.
 */
NTSTATUS PhCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName,
    _In_ ULONG Attributes,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    if (!NT_SUCCESS(status = PhpInitializeKeyObjectAttributes(
        RootDirectory,
        &objectName,
        Attributes,
        &objectAttributes
        )))
    {
        return status;
    }

    status = NtCreateKey(
        KeyHandle,
        DesiredAccess,
        &objectAttributes,
        0,
        NULL,
        CreateOptions,
        Disposition
        );

    return status;
}

/**
 * Opens a registry key.
 *
 * \param KeyHandle A variable which receives a handle to the key.
 * \param DesiredAccess The desired access to the key.
 * \param RootDirectory A handle to a root key, or one of the predefined keys. See PhCreateKey() for
 * details.
 * \param ObjectName The path to the key.
 * \param Attributes Additional object flags.
 */
NTSTATUS PhOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName,
    _In_ ULONG Attributes
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    if (!NT_SUCCESS(status = PhpInitializeKeyObjectAttributes(
        RootDirectory,
        &objectName,
        Attributes,
        &objectAttributes
        )))
    {
        return status;
    }

    status = NtOpenKey(
        KeyHandle,
        DesiredAccess,
        &objectAttributes
        );

    return status;
}

// rev from RegLoadAppKey
/**
 * Loads the specified registry hive file into a private application hive.
 *
 * \param KeyHandle A variable which receives a handle to the key.
 * \param FileName A string containing a file name.
 * \param DesiredAccess The desired access to the key.
 * \param Flags Optional flags for loading the hive.
 */
NTSTATUS PhLoadAppKey(
    _Out_ PHANDLE KeyHandle,
    _In_ PCPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG Flags
    )
{
    NTSTATUS status;
    GUID guid;
    UNICODE_STRING fileName;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES targetAttributes;
    OBJECT_ATTRIBUTES sourceAttributes;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    PhGenerateGuid(&guid);

#if defined(PHNT_USE_NATIVE_APPEND)
    UNICODE_STRING guidStringUs;
    WCHAR objectNameBuffer[MAXIMUM_FILENAME_LENGTH];

    RtlInitEmptyUnicodeString(&objectName, objectNameBuffer, sizeof(objectNameBuffer));

    if (!NT_SUCCESS(status = RtlStringFromGUID(&guid, &guidStringUs)))
        return status;

    if (!NT_SUCCESS(status = RtlAppendUnicodeToString(&objectName, L"\\REGISTRY\\A\\")))
        goto CleanupExit;

    if (!NT_SUCCESS(status = RtlAppendUnicodeStringToString(&objectName, &guidStringUs)))
        goto CleanupExit;
#else
    static CONST PH_STRINGREF namespaceString = PH_STRINGREF_INIT(L"\\REGISTRY\\A\\");
    PPH_STRING guidString;

    if (!(guidString = PhFormatGuid(&guid)))
        return STATUS_UNSUCCESSFUL;

    PhMoveReference(&guidString, PhConcatStringRef2(&namespaceString, &guidString->sr));

    if (!PhStringRefToUnicodeString(&guidString->sr, &objectName))
    {
        PhDereferenceObject(guidString);
        return STATUS_NAME_TOO_LONG;
    }
#endif

    InitializeObjectAttributes(
        &targetAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    InitializeObjectAttributes(
        &sourceAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtLoadKeyEx(
        &targetAttributes,
        &sourceAttributes,
        REG_APP_HIVE | Flags,
        NULL,
        NULL,
        DesiredAccess,
        KeyHandle,
        NULL
        );

#if defined(PHNT_USE_NATIVE_APPEND)
    RtlFreeUnicodeString(&guidStringUs);
#else
    PhDereferenceObject(guidString);
#endif

    return status;
}

/**
 * Gets information about a registry key.
 *
 * \param KeyHandle A handle to the key.
 * \param KeyInformationClass The information class to query.
 * \param Buffer A variable which receives a pointer to a buffer containing information about the
 * registry key. You must free the buffer with PhFree() when you no longer need it.
 */
NTSTATUS PhQueryKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS KeyInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts = 16;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    do
    {
        status = NtQueryKey(
            KeyHandle,
            KeyInformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            PhFree(buffer);
            return status;
        }
    } while (--attempts);

    *Buffer = buffer;

    return status;
}

/**
 * Gets the information for a registry key.
 *
 * \param KeyHandle A handle to the key.
 * \param Information The registry key information, including information
 * about its subkeys and the maximum length for their names and value entries.
 */
NTSTATUS PhQueryKeyInformation(
    _In_ HANDLE KeyHandle,
    _Out_opt_ PKEY_FULL_INFORMATION Information
    )
{
    NTSTATUS status;
    ULONG bufferLength = 0;

    status = NtQueryKey(
        KeyHandle,
        KeyFullInformation,
        Information,
        sizeof(KEY_FULL_INFORMATION),
        &bufferLength
        );

    return status;
}

/**
 * Gets the last write time for a registry key without allocating memory. (dmex)
 *
 * \param KeyHandle A handle to the key.
 * \param LastWriteTime The last write time of the key.
 */
NTSTATUS PhQueryKeyLastWriteTime(
    _In_ HANDLE KeyHandle,
    _Out_ PLARGE_INTEGER LastWriteTime
    )
{
    NTSTATUS status;
    KEY_BASIC_INFORMATION basicInfo = { 0 };
    ULONG bufferLength = 0;

    status = NtQueryKey(
        KeyHandle,
        KeyBasicInformation,
        &basicInfo,
        UFIELD_OFFSET(KEY_BASIC_INFORMATION, Name),
        &bufferLength
        );

    if (status == STATUS_BUFFER_OVERFLOW && basicInfo.LastWriteTime.QuadPart != 0)
    {
        *LastWriteTime = basicInfo.LastWriteTime;
        return STATUS_SUCCESS;
    }
    else
    {
        PKEY_BASIC_INFORMATION buffer;

        status = PhQueryKey(
            KeyHandle,
            KeyBasicInformation,
            &buffer
            );

        if (NT_SUCCESS(status))
        {
            memcpy(LastWriteTime, &buffer->LastWriteTime, sizeof(LARGE_INTEGER));
            PhFree(buffer);
        }
    }

    return status;
}

/**
 * Gets a registry value of any type.
 *
 * \param KeyHandle A handle to the key.
 * \param ValueName The name of the value.
 * \param KeyValueInformationClass The information class to query.
 * \param Buffer A variable which receives a pointer to a buffer containing information about the
 * registry value. You must free the buffer with PhFree() when you no longer need it.
 */
NTSTATUS PhQueryValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PCPH_STRINGREF ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    UNICODE_STRING valueName;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts = 16;

    if (ValueName && ValueName->Length)
    {
        if (!PhStringRefToUnicodeString(ValueName, &valueName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&valueName, NULL, 0);
    }

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    do
    {
        status = NtQueryValueKey(
            KeyHandle,
            &valueName,
            KeyValueInformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            PhFree(buffer);
            return status;
        }
    } while (--attempts);

    *Buffer = buffer;

    return status;
}

NTSTATUS PhSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PCPH_STRINGREF ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    NTSTATUS status;
    UNICODE_STRING valueName;

    if (ValueName)
    {
        if (!PhStringRefToUnicodeString(ValueName, &valueName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&valueName, NULL, 0);
    }

    status = NtSetValueKey(
        KeyHandle,
        &valueName,
        0,
        ValueType,
        Buffer,
        BufferLength
        );

    return status;
}

NTSTATUS PhDeleteValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PCPH_STRINGREF ValueName
    )
{
    UNICODE_STRING valueName;

    if (ValueName)
    {
        if (!PhStringRefToUnicodeString(ValueName, &valueName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&valueName, NULL, 0);
    }

    return NtDeleteValueKey(KeyHandle, &valueName);
}

NTSTATUS PhEnumerateKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS InformationClass,
    _In_ PPH_ENUM_KEY_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG index = 0;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtEnumerateKey(
            KeyHandle,
            index,
            InformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_NO_MORE_ENTRIES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);

            status = NtEnumerateKey(
                KeyHandle,
                index,
                InformationClass,
                buffer,
                bufferSize,
                &bufferSize
                );
        }

        if (!NT_SUCCESS(status))
            break;

        if (!Callback(KeyHandle, buffer, Context))
            break;

        index++;
    }

    PhFree(buffer);

    return status;
}

NTSTATUS PhEnumerateValueKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_VALUE_INFORMATION_CLASS InformationClass,
    _In_ PPH_ENUM_KEY_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG index = 0;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtEnumerateValueKey(
            KeyHandle,
            index,
            InformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_NO_MORE_ENTRIES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);

            status = NtEnumerateValueKey(
                KeyHandle,
                index,
                InformationClass,
                buffer,
                bufferSize,
                &bufferSize
                );
        }

        if (!NT_SUCCESS(status))
            break;

        if (!Callback(KeyHandle, buffer, Context))
            break;

        index++;
    }

    PhFree(buffer);

    return status;
}

/**
 * Queries file attributes.
 *
 * \param FileName The Win32 file name.
 * \param FileInformation A variable that receives the file information.
 */
NTSTATUS PhQueryFullAttributesFileWin32(
    _In_ PCWSTR FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryFullAttributesFile(&objectAttributes, FileInformation);

    RtlFreeUnicodeString(&fileName);

    return status;
}

NTSTATUS PhQueryFullAttributesFile(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryFullAttributesFile(&objectAttributes, FileInformation);

    return status;
}

NTSTATUS PhQueryAttributesFileWin32(
    _In_ PCWSTR FileName,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        FileName,
        &fileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryAttributesFile(&objectAttributes, FileInformation);

    RtlFreeUnicodeString(&fileName);

    return status;
}

NTSTATUS PhQueryAttributesFile(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PFILE_BASIC_INFORMATION FileInformation
    )
{
    NTSTATUS status;
    UNICODE_STRING fileName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtQueryAttributesFile(&objectAttributes, FileInformation);

    return status;
}

// rev from RtlDoesFileExists_U (dmex)
BOOLEAN PhDoesFileExistWin32(
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFileWin32(FileName, &basicInfo);

    if (
        NT_SUCCESS(status) ||
        status == STATUS_SHARING_VIOLATION ||
        status == STATUS_ACCESS_DENIED
        )
    {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN PhDoesFileExist(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFile(FileName, &basicInfo);

    if (
        NT_SUCCESS(status) ||
        status == STATUS_SHARING_VIOLATION ||
        status == STATUS_ACCESS_DENIED
        )
    {
        return TRUE;
    }

    return FALSE;
}

BOOLEAN PhDoesDirectoryExistWin32(
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFileWin32(FileName, &basicInfo);

    if (NT_SUCCESS(status))
    {
        if (basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            return TRUE;
    }

    return FALSE;
}

BOOLEAN PhDoesDirectoryExist(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    FILE_BASIC_INFORMATION basicInfo;

    status = PhQueryAttributesFile(FileName, &basicInfo);

    if (NT_SUCCESS(status))
    {
        if (basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            return TRUE;
    }

    return FALSE;
}

// rev from RtlDetermineDosPathNameType_U (dmex)
RTL_PATH_TYPE PhDetermineDosPathNameType(
    _In_ PCPH_STRINGREF FileName
    )
{
#if defined(PHNT_USE_NATIVE_PATHTYPE)
    return RtlDetermineDosPathNameType_U(FileName);
#else
    if (FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[0] == OBJ_NAME_ALTPATH_SEPARATOR)
    {
        if (FileName->Buffer[1] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[1] == OBJ_NAME_ALTPATH_SEPARATOR)
        {
            if (FileName->Buffer[2] == L'?' || FileName->Buffer[2] == L'.')
            {
                if (FileName->Buffer[3] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[3] == OBJ_NAME_ALTPATH_SEPARATOR)
                    return RtlPathTypeLocalDevice;

                if (FileName->Buffer[3] != UNICODE_NULL)
                    return RtlPathTypeUncAbsolute;

                return RtlPathTypeRootLocalDevice;
            }

            return RtlPathTypeUncAbsolute;
        }

        return RtlPathTypeRooted;
    }
    else if (FileName->Buffer[0] != UNICODE_NULL && FileName->Buffer[1] == L':')
    {
        if (FileName->Buffer[2] == OBJ_NAME_PATH_SEPARATOR || FileName->Buffer[2] == OBJ_NAME_ALTPATH_SEPARATOR)
            return RtlPathTypeDriveAbsolute;

        return RtlPathTypeDriveRelative;
    }

    return RtlPathTypeRelative;
#endif
}

/**
 * Deletes a file.
 *
 * \param FileName The Win32 file name.
 */
NTSTATUS PhDeleteFileWin32(
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
    //UNICODE_STRING fileName;
    //OBJECT_ATTRIBUTES objectAttributes;
    //
    //status = PhDosLongPathNameToNtPathNameWithStatus(
    //    FileName,
    //    &fileName,
    //    NULL,
    //    NULL
    //    );
    //
    //if (!NT_SUCCESS(status))
    //    return status;
    //
    //InitializeObjectAttributes(
    //    &objectAttributes,
    //    &fileName,
    //    OBJ_CASE_INSENSITIVE,
    //    NULL,
    //    NULL
    //    );
    //
    //status = NtDeleteFile(&objectAttributes);
    //
    //RtlFreeUnicodeString(&fileName);
    //
    //if (!NT_SUCCESS(status))
    {
        HANDLE fileHandle;

        status = PhCreateFileWin32(
            &fileHandle,
            FileName,
            DELETE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DELETE_ON_CLOSE
            );

        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_OBJECT_NAME_NOT_FOUND)
                status = STATUS_SUCCESS;
            return status;
        }

        //PhSetFileDelete(fileHandle);

        NtClose(fileHandle);
    }

    return status;
}

NTSTATUS PhDeleteFile(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    //UNICODE_STRING fileName;
    //OBJECT_ATTRIBUTES objectAttributes;
    //
    //if (!NT_SUCCESS(status))
    //    return status;
    //
    //InitializeObjectAttributes(
    //    &objectAttributes,
    //    &fileName,
    //    OBJ_CASE_INSENSITIVE,
    //    NULL,
    //    NULL
    //    );
    //
    //status = NtDeleteFile(&objectAttributes);
    //
    //RtlFreeUnicodeString(&fileName);
    //
    //if (!NT_SUCCESS(status))
    {
        HANDLE fileHandle;

        status = PhCreateFile(
            &fileHandle,
            FileName,
            DELETE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_DELETE_ON_CLOSE
            );

        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_OBJECT_NAME_NOT_FOUND)
                status = STATUS_SUCCESS;
            return status;
        }

        NtClose(fileHandle);
    }

    return status;
}

/**
* Creates a directory path recursively.
*
* \param DirectoryPath The directory path.
*/
NTSTATUS PhCreateDirectory(
    _In_ PCPH_STRINGREF DirectoryPath
    )
{
    PPH_STRING directoryPath;
    PPH_STRING directoryName;
    PH_STRINGREF directoryPart;
    PH_STRINGREF remainingPart;

    if (PhDoesDirectoryExist(DirectoryPath))
        return STATUS_SUCCESS;

    directoryPath = PhGetExistingPathPrefix(DirectoryPath);

    if (PhIsNullOrEmptyString(directoryPath))
        return STATUS_UNSUCCESSFUL;

    remainingPart.Length = DirectoryPath->Length - directoryPath->Length - sizeof(OBJ_NAME_PATH_SEPARATOR);
    remainingPart.Buffer = PTR_ADD_OFFSET(DirectoryPath->Buffer, directoryPath->Length + sizeof(OBJ_NAME_PATH_SEPARATOR));

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

        if (directoryPart.Length != 0)
        {
            directoryName = PhConcatStringRef3(
                &directoryPath->sr,
                &PhNtPathSeperatorString,
                &directoryPart
                );

            if (!PhDoesDirectoryExist(&directoryName->sr))
            {
                HANDLE directoryHandle;

                if (NT_SUCCESS(PhCreateFile(
                    &directoryHandle,
                    &directoryName->sr,
                    FILE_LIST_DIRECTORY | SYNCHRONIZE,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_CREATE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT //| FILE_OPEN_REPARSE_POINT
                    )))
                {
                    NtClose(directoryHandle);
                }
            }

            PhMoveReference(&directoryPath, directoryName);
        }
    }

    PhClearReference(&directoryPath);

    if (!PhDoesDirectoryExist(DirectoryPath))
        return STATUS_NOT_FOUND;

    return STATUS_SUCCESS;
}

/**
* Creates a directory path recursively.
*
* \param DirectoryPath The Win32 directory path.
*/
NTSTATUS PhCreateDirectoryWin32(
    _In_ PCPH_STRINGREF DirectoryPath
    )
{
    PPH_STRING directoryPath;
    PPH_STRING directoryName;
    PH_STRINGREF directoryPart;
    PH_STRINGREF remainingPart;

    if (PhDoesDirectoryExistWin32(PhGetStringRefZ(DirectoryPath)))
        return STATUS_SUCCESS;

    directoryPath = PhGetExistingPathPrefixWin32(DirectoryPath);

    if (PhIsNullOrEmptyString(directoryPath))
        return STATUS_UNSUCCESSFUL;

    remainingPart.Length = DirectoryPath->Length - directoryPath->Length - sizeof(OBJ_NAME_PATH_SEPARATOR);
    remainingPart.Buffer = PTR_ADD_OFFSET(DirectoryPath->Buffer, directoryPath->Length + sizeof(OBJ_NAME_PATH_SEPARATOR));

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &remainingPart);

        if (directoryPart.Length != 0)
        {
            if (PhIsNullOrEmptyString(directoryPath))
            {
                PhMoveReference(&directoryPath, PhCreateString2(&directoryPart));
            }
            else
            {
                directoryName = PhConcatStringRef3(&directoryPath->sr, &PhNtPathSeperatorString, &directoryPart);

                // Check if the directory already exists. (dmex)

                if (!PhDoesDirectoryExistWin32(PhGetString(directoryName)))
                {
                    HANDLE directoryHandle;

                    // Create the directory. (dmex)

                    if (NT_SUCCESS(PhCreateFileWin32(
                        &directoryHandle,
                        PhGetString(directoryName),
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_CREATE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT //| FILE_OPEN_REPARSE_POINT
                        )))
                    {
                        NtClose(directoryHandle);
                    }
                }

                PhMoveReference(&directoryPath, directoryName);
            }
        }
    }

    PhClearReference(&directoryPath);

    if (!PhDoesDirectoryExistWin32(PhGetStringRefZ(DirectoryPath)))
        return STATUS_NOT_FOUND;

    return STATUS_SUCCESS;
}

NTSTATUS PhCreateDirectoryFullPathWin32(
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PH_STRINGREF directoryPart;
    PPH_STRING directoryPath;
    PPH_STRING directory;

    if (PhGetBasePath(FileName, &directoryPart, NULL))
    {
        if (directory = PhCreateString2(&directoryPart))
        {
            if (NT_SUCCESS(PhGetFullPath(PhGetString(directory), &directoryPath, NULL)))
            {
                status = PhCreateDirectoryWin32(&directoryPath->sr);
                PhDereferenceObject(directoryPath);
            }

            PhDereferenceObject(directory);
        }
    }

    return status;
}

NTSTATUS PhCreateDirectoryFullPath(
    _In_ PCPH_STRINGREF FileName
    )
{
    PH_STRINGREF directoryPart;

    if (PhGetBasePath(FileName, &directoryPart, NULL))
    {
        return PhCreateDirectory(&directoryPart);
    }

    return STATUS_UNSUCCESSFUL;
}

// NOTE: This callback handles both Native and Win32 filenames
// since they're both relative to the parent RootDirectory. (dmex)
_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN PhDeleteDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PH_STRINGREF fileName;
    HANDLE fileHandle;

    fileName.Buffer = Information->FileName;
    fileName.Length = Information->FileNameLength;

    if (FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        if (PATH_IS_WIN32_RELATIVE_PREFIX(&fileName))
            return TRUE;

        if (NT_SUCCESS(PhCreateFileEx(
            &fileHandle,
            &fileName,
            FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
            RootDirectory,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT, // | FILE_OPEN_REPARSE_POINT
            NULL
            )))
        {
            PhEnumDirectoryFile(fileHandle, NULL, PhDeleteDirectoryCallback, NULL);

            PhSetFileDelete(fileHandle);

            NtClose(fileHandle);
        }
    }
    else
    {
        if (NT_SUCCESS(PhCreateFileEx(
            &fileHandle,
            &fileName,
            FILE_WRITE_ATTRIBUTES | DELETE | SYNCHRONIZE,
            RootDirectory,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, // | FILE_OPEN_REPARSE_POINT
            NULL
            )))
        {
            if (FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_READONLY) && WindowsVersion < WINDOWS_10_RS5)
            {
                FILE_BASIC_INFORMATION fileBasicInfo;

                memset(&fileBasicInfo, 0, sizeof(FILE_BASIC_INFORMATION));
                fileBasicInfo.FileAttributes = ClearFlag(Information->FileAttributes, FILE_ATTRIBUTE_READONLY);

                PhSetFileBasicInformation(fileHandle, &fileBasicInfo);
            }

            PhSetFileDelete(fileHandle);

            NtClose(fileHandle);
        }
    }

    return TRUE;
}

/**
* Deletes a directory path recursively.
*
* \param DirectoryPath The directory path.
*/
NTSTATUS PhDeleteDirectory(
    _In_ PCPH_STRINGREF DirectoryPath
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;

    status = PhCreateFile(
        &directoryHandle,
        DirectoryPath,
        FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
        );

    if (NT_SUCCESS(status))
    {
        // Remove any files or folders inside the directory. (dmex)
        status = PhEnumDirectoryFile(
            directoryHandle,
            NULL,
            PhDeleteDirectoryCallback,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            // Remove the directory. (dmex)
            status = PhSetFileDelete(directoryHandle);
        }

        NtClose(directoryHandle);
    }

    if (!PhDoesDirectoryExist(DirectoryPath))
        return STATUS_SUCCESS;

    return status;
}

/**
* Deletes a directory path recursively.
*
* \param DirectoryPath The Win32 directory path.
*/
NTSTATUS PhDeleteDirectoryWin32(
    _In_ PCPH_STRINGREF DirectoryPath
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;

    status = PhCreateFileWin32(
        &directoryHandle,
        PhGetStringRefZ(DirectoryPath),
        FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
        );

    if (NT_SUCCESS(status))
    {
        // Remove any files or folders inside the directory. (dmex)
        status = PhEnumDirectoryFile(
            directoryHandle,
            NULL,
            PhDeleteDirectoryCallback,
            (PVOID)DirectoryPath
            );

        if (NT_SUCCESS(status))
        {
            // Remove the directory. (dmex)
            status = PhSetFileDelete(directoryHandle);
        }

        NtClose(directoryHandle);
    }

    if (!PhDoesDirectoryExistWin32(PhGetStringRefZ(DirectoryPath)))
        return STATUS_SUCCESS;

    return status;
}

NTSTATUS PhDeleteDirectoryFullPath(
    _In_ PCPH_STRINGREF FileName
    )
{
    PH_STRINGREF directoryPart;

    if (PhGetBasePath(FileName, &directoryPart, NULL))
    {
        return PhDeleteDirectory(&directoryPart);
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhCopyFileWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    HANDLE newFileHandle;
    FILE_BASIC_INFORMATION basicInfo;
    LARGE_INTEGER newFileSize;
    IO_STATUS_BLOCK isb;
    PBYTE buffer;

    status = PhCreateFileWin32(
        &fileHandle,
        OldFileName,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileBasicInformation(fileHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileSize(fileHandle, &newFileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateFileWin32Ex(
        &newFileHandle,
        NewFileName,
        FILE_GENERIC_WRITE,
        &newFileSize,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    buffer = PhAllocatePage(PAGE_SIZE * 2, NULL);

    if (!buffer)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    while (TRUE)
    {
        status = NtReadFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            PAGE_SIZE * 2,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;
        if (isb.Information == 0)
            break;

        status = NtWriteFile(
            newFileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            (ULONG)isb.Information,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;
        if (isb.Information == 0)
            break;
    }

    PhFreePage(buffer);

    if (status == STATUS_END_OF_FILE)
    {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status))
    {
        PhSetFileBasicInformation(
            newFileHandle,
            &basicInfo
            );
    }
    else
    {
        PhSetFileDelete(newFileHandle);
    }

    NtClose(newFileHandle);

CleanupExit:
    NtClose(fileHandle);

    return status;
}

NTSTATUS PhCopyFileChunkDirectIoWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE sourceHandle;
    HANDLE destinationHandle;
    FILE_BASIC_INFORMATION basicInfo;
    FILE_FS_SECTOR_SIZE_INFORMATION sourceSectorInfo = { 0 };
    FILE_FS_SECTOR_SIZE_INFORMATION destinationSectorInfo = { 0 };
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER sourceOffset = { 0 };
    LARGE_INTEGER destinationOffset = { 0 };
    LARGE_INTEGER fileSize;
    SIZE_T numberOfBytes;
    ULONG alignSize;
    ULONG blockSize;

    if (!NtCopyFileChunk_Import())
        return STATUS_NOT_SUPPORTED;

    status = PhCreateFileWin32ExAlt(
        &sourceHandle,
        OldFileName,
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING,
        EX_CREATE_FLAG_FILE_SOURCE_OPEN_FOR_COPY,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileBasicInformation(sourceHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileSize(sourceHandle, &fileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateFileWin32ExAlt(
        &destinationHandle,
        NewFileName,
        FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING,
        EX_CREATE_FLAG_FILE_DEST_OPEN_FOR_COPY,
        &fileSize,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // https://learn.microsoft.com/en-us/windows/win32/w8cookbook/advanced-format--4k--disk-compatibility-update
    NtQueryVolumeInformationFile(
        sourceHandle,
        &ioStatusBlock,
        &sourceSectorInfo,
        sizeof(FILE_FS_SECTOR_SIZE_INFORMATION),
        FileFsSectorSizeInformation
        );

    NtQueryVolumeInformationFile(
        destinationHandle,
        &ioStatusBlock,
        &destinationSectorInfo,
        sizeof(FILE_FS_SECTOR_SIZE_INFORMATION),
        FileFsSectorSizeInformation
        );

    // Non-cached I/O requires 'blockSize' be sector-aligned with whichever file is opened as non-cached.
    // If both, the length should be aligned with the larger sector size of the two. (dmex)
    alignSize = __max(max(sourceSectorInfo.PhysicalBytesPerSectorForPerformance, destinationSectorInfo.PhysicalBytesPerSectorForPerformance),
        max(sourceSectorInfo.PhysicalBytesPerSectorForAtomicity, destinationSectorInfo.PhysicalBytesPerSectorForAtomicity));

    // Enable BypassIO (skip error checking since might be disabled) (dmex)
    PhSetFileBypassIO(sourceHandle, TRUE);
    PhSetFileBypassIO(destinationHandle, TRUE);

    blockSize = PAGE_SIZE;
    numberOfBytes = (SIZE_T)fileSize.QuadPart;

    while (numberOfBytes != 0)
    {
        if (blockSize > numberOfBytes)
            blockSize = (ULONG)numberOfBytes;
        blockSize = ALIGN_UP_BY(blockSize, alignSize);

        status = NtCopyFileChunk_Import()(
            sourceHandle,
            destinationHandle,
            NULL,
            &ioStatusBlock,
            blockSize,
            &sourceOffset,
            &destinationOffset,
            NULL,
            NULL,
            0
            );

        if (!NT_SUCCESS(status))
            break;

        destinationOffset.QuadPart += blockSize;
        sourceOffset.QuadPart += blockSize;
        numberOfBytes -= blockSize;
    }

    if (status == STATUS_END_OF_FILE)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
    {
        status = PhSetFileSize(destinationHandle, &fileSize); // Required (dmex)
    }

    if (NT_SUCCESS(status))
    {
        status = PhSetFileBasicInformation(destinationHandle, &basicInfo);
    }

    if (!NT_SUCCESS(status))
    {
        PhSetFileDelete(destinationHandle);
    }

    NtClose(destinationHandle);

CleanupExit:
    NtClose(sourceHandle);

    return status;
}

NTSTATUS PhCopyFileChunkWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE sourceHandle;
    HANDLE destinationHandle;
    FILE_BASIC_INFORMATION basicInfo;
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER sourceOffset = { 0 };
    LARGE_INTEGER destinationOffset = { 0 };
    LARGE_INTEGER fileSize;

    if (!NtCopyFileChunk_Import())
        return STATUS_NOT_SUPPORTED;

    status = PhCreateFileWin32ExAlt(
        &sourceHandle,
        OldFileName,
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        EX_CREATE_FLAG_FILE_SOURCE_OPEN_FOR_COPY,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileBasicInformation(sourceHandle, &basicInfo);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileSize(sourceHandle, &fileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhCreateFileWin32ExAlt(
        &destinationHandle,
        NewFileName,
        FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        EX_CREATE_FLAG_FILE_DEST_OPEN_FOR_COPY,
        &fileSize,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (fileSize.QuadPart >= ULONG_MAX)
    {
        SIZE_T numberOfBytes = (SIZE_T)fileSize.QuadPart;
        ULONG blockSize = ULONG_MAX;

        // Split into smaller blocks when the length
        // overflows the maximum chunk size. (dmex)

        while (numberOfBytes != 0)
        {
            if (blockSize > numberOfBytes)
                blockSize = (ULONG)numberOfBytes;

            status = NtCopyFileChunk_Import()(
                sourceHandle,
                destinationHandle,
                NULL,
                &ioStatusBlock,
                blockSize,
                &sourceOffset,
                &destinationOffset,
                NULL,
                NULL,
                0
                );

            if (!NT_SUCCESS(status))
                break;

            destinationOffset.QuadPart += blockSize;
            sourceOffset.QuadPart += blockSize;
            numberOfBytes -= blockSize;
        }
    }
    else
    {
        status = NtCopyFileChunk_Import()(
            sourceHandle,
            destinationHandle,
            NULL,
            &ioStatusBlock,
            (ULONG)fileSize.QuadPart,
            &sourceOffset,
            &destinationOffset,
            NULL,
            NULL,
            0
            );
    }

    if (NT_SUCCESS(status))
    {
        PhSetFileBasicInformation(
            destinationHandle,
            &basicInfo
            );
    }
    else
    {
        PhSetFileDelete(destinationHandle);
    }

    NtClose(destinationHandle);

CleanupExit:
    NtClose(sourceHandle);

    return status;
}

NTSTATUS PhMoveFileWin32(
    _In_ PCWSTR OldFileName,
    _In_ PCWSTR NewFileName,
    _In_ BOOLEAN FailIfExists
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    ULONG renameInfoLength;
    UNICODE_STRING newFileName;
    PFILE_RENAME_INFORMATION renameInfo;

    status = PhDosLongPathNameToNtPathNameWithStatus(
        NewFileName,
        &newFileName,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhCreateFileWin32(
        &fileHandle,
        OldFileName,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | DELETE | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        );

    if (!NT_SUCCESS(status))
    {
        RtlFreeUnicodeString(&newFileName);
        return status;
    }

    renameInfoLength = sizeof(FILE_RENAME_INFORMATION) + newFileName.Length + sizeof(UNICODE_NULL);
    renameInfo = PhAllocateZero(renameInfoLength);
    renameInfo->ReplaceIfExists = FailIfExists ? FALSE : TRUE;
    renameInfo->RootDirectory = NULL;
    renameInfo->FileNameLength = newFileName.Length;
    memcpy(renameInfo->FileName, newFileName.Buffer, newFileName.Length);
    RtlFreeUnicodeString(&newFileName);

    status = NtSetInformationFile(
        fileHandle,
        &isb,
        renameInfo,
        renameInfoLength,
        FileRenameInformation
        );

    if (status == STATUS_NOT_SAME_DEVICE)
    {
        HANDLE newFileHandle;
        LARGE_INTEGER newFileSize;
        PBYTE buffer;

        status = PhGetFileSize(fileHandle, &newFileSize);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhCreateFileWin32Ex(
            &newFileHandle,
            NewFileName,
            FILE_GENERIC_WRITE,
            &newFileSize,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_DELETE,
            FailIfExists ? FILE_CREATE : FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            buffer = PhAllocatePage(PAGE_SIZE * 2, NULL);

            if (!buffer)
            {
                status = STATUS_NO_MEMORY;
                goto CleanupExit;
            }

            while (TRUE)
            {
                status = NtReadFile(
                    fileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    PAGE_SIZE * 2,
                    NULL,
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;
                if (isb.Information == 0)
                    break;

                status = NtWriteFile(
                    newFileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &isb,
                    buffer,
                    (ULONG)isb.Information,
                    NULL,
                    NULL
                    );

                if (!NT_SUCCESS(status))
                    break;
                if (isb.Information == 0)
                    break;
            }

            PhFreePage(buffer);

            if (status == STATUS_END_OF_FILE)
            {
                status = STATUS_SUCCESS;
            }

            if (status != STATUS_SUCCESS)
            {
                PhSetFileDelete(newFileHandle);
            }

            NtClose(newFileHandle);
        }
    }

CleanupExit:
    NtClose(fileHandle);
    PhFree(renameInfo);

    return status;
}

NTSTATUS PhGetProcessHeapSignature(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID HeapAddress,
    _In_ ULONG IsWow64,
    _Out_ ULONG *HeapSignature
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG heapSignature = ULONG_MAX;

    if (WindowsVersion >= WINDOWS_7)
    {
        // dt _HEAP SegmentSignature
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x8 : 0x10),
            &heapSignature,
            sizeof(ULONG),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        if (HeapSignature)
            *HeapSignature = heapSignature;
    }

    return status;
}

NTSTATUS PhGetProcessHeapFrontEndType(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID HeapAddress,
    _In_ ULONG IsWow64,
    _Out_ UCHAR *HeapFrontEndType
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    UCHAR heapFrontEndType = UCHAR_MAX;

    if (WindowsVersion >= WINDOWS_10)
    {
        // dt _HEAP FrontEndHeapType
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x0ea : 0x1a2),
            &heapFrontEndType,
            sizeof(UCHAR),
            NULL
            );
    }
    else if (WindowsVersion >= WINDOWS_8_1)
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x0d6 : 0x17a),
            &heapFrontEndType,
            sizeof(UCHAR),
            NULL
            );
    }
    else if (WindowsVersion >= WINDOWS_7)
    {
        status = NtReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x0da : 0x182),
            &heapFrontEndType,
            sizeof(UCHAR),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        if (HeapFrontEndType)
            *HeapFrontEndType = heapFrontEndType;
    }

    return status;
}

NTSTATUS PhQueryProcessHeapInformation(
    _In_ HANDLE ProcessId,
    _Out_ PPH_PROCESS_DEBUG_HEAP_INFORMATION* HeapInformation
    )
{
    NTSTATUS status;
    PRTL_DEBUG_INFORMATION debugBuffer = NULL;
    PPH_PROCESS_DEBUG_HEAP_INFORMATION heapDebugInfo = NULL;

    for (ULONG i = 0x400000; ; i *= 2) // rev from Heap32First/Heap32Next (dmex)
    {
        if (!(debugBuffer = RtlCreateQueryDebugBuffer(i, FALSE)))
            return STATUS_UNSUCCESSFUL;

        status = RtlQueryProcessDebugInformation(
            ProcessId,
            RTL_QUERY_PROCESS_HEAP_SUMMARY | RTL_QUERY_PROCESS_HEAP_ENTRIES | RTL_QUERY_PROCESS_NONINVASIVE,
            debugBuffer
            );

        if (!NT_SUCCESS(status))
        {
            RtlDestroyQueryDebugBuffer(debugBuffer);
            debugBuffer = NULL;
        }

        if (NT_SUCCESS(status) || status != STATUS_NO_MEMORY)
            break;

        if (2 * i <= i)
        {
            status = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    if (!NT_SUCCESS(status))
        return status;

    if (!debugBuffer->Heaps)
    {
        // The RtlQueryProcessDebugInformation function has two bugs on some versions
        // when querying the ProcessId for a frozen (suspended) immersive process. (dmex)
        //
        // 1) It'll deadlock the current thread for 30 seconds.
        // 2) It'll return STATUS_SUCCESS but with a NULL Heaps buffer.
        //
        // A workaround was implemented using PhCreateExecutionRequiredRequest() (dmex)

        RtlDestroyQueryDebugBuffer(debugBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    if (WindowsVersion > WINDOWS_11)
    {
        heapDebugInfo = PhAllocateZero(sizeof(PH_PROCESS_DEBUG_HEAP_INFORMATION) + ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->NumberOfHeaps * sizeof(PH_PROCESS_DEBUG_HEAP_ENTRY));
        heapDebugInfo->NumberOfHeaps = ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->NumberOfHeaps;
    }
    else
    {
        heapDebugInfo = PhAllocateZero(sizeof(PH_PROCESS_DEBUG_HEAP_INFORMATION) + ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->NumberOfHeaps * sizeof(PH_PROCESS_DEBUG_HEAP_ENTRY));
        heapDebugInfo->NumberOfHeaps = ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->NumberOfHeaps;
    }

    heapDebugInfo->DefaultHeap = debugBuffer->ProcessHeap;

    for (ULONG i = 0; i < heapDebugInfo->NumberOfHeaps; i++)
    {
        RTL_HEAP_INFORMATION_V2 heapInfo = { 0 };
        HANDLE processHandle;
        SIZE_T allocated = 0;
        SIZE_T committed = 0;

        if (WindowsVersion > WINDOWS_11)
        {
            heapInfo = ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->Heaps[i];
        }
        else
        {
            RTL_HEAP_INFORMATION_V1 heapInfoV1 = ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->Heaps[i];
            heapInfo.NumberOfEntries = heapInfoV1.NumberOfEntries;
            heapInfo.Entries = heapInfoV1.Entries;
            heapInfo.BytesCommitted = heapInfoV1.BytesCommitted;
            heapInfo.Flags = heapInfoV1.Flags;
            heapInfo.BaseAddress = heapInfoV1.BaseAddress;
        }

        // go through all heap entries and compute amount of allocated and committed bytes (ge0rdi)
        for (ULONG e = 0; e < heapInfo.NumberOfEntries; e++)
        {
            PRTL_HEAP_ENTRY entry = &heapInfo.Entries[e];

            if (entry->Flags & RTL_HEAP_BUSY)
                allocated += entry->Size;
            if (entry->Flags & RTL_HEAP_SEGMENT)
                committed += entry->u.s2.CommittedSize;
        }

        // sometimes computed number if committed bytes is few pages smaller than the one reported by API, lets use the higher value (ge0rdi)
        if (committed < heapInfo.BytesCommitted)
            committed = heapInfo.BytesCommitted;

        // make sure number of allocated bytes is not higher than number of committed bytes (as that would make no sense) (ge0rdi)
        if (allocated > committed)
            allocated = committed;

        heapDebugInfo->Heaps[i].Flags = heapInfo.Flags;
        heapDebugInfo->Heaps[i].Signature = ULONG_MAX;
        heapDebugInfo->Heaps[i].HeapFrontEndType = UCHAR_MAX;
        heapDebugInfo->Heaps[i].NumberOfEntries = heapInfo.NumberOfEntries;
        heapDebugInfo->Heaps[i].BaseAddress = heapInfo.BaseAddress;
        heapDebugInfo->Heaps[i].BytesAllocated = allocated;
        heapDebugInfo->Heaps[i].BytesCommitted = committed;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
            ProcessId
            )))
        {
            ULONG signature = ULONG_MAX;
            UCHAR frontEndType = UCHAR_MAX;
#ifndef _WIN64
            BOOLEAN isWow64 = TRUE;
#else
            BOOLEAN isWow64 = FALSE;

            PhGetProcessIsWow64(processHandle, &isWow64);
#endif
            if (NT_SUCCESS(PhGetProcessHeapSignature(
                processHandle,
                heapInfo.BaseAddress,
                isWow64,
                &signature
                )))
            {
                heapDebugInfo->Heaps[i].Signature = signature;
            }

            if (NT_SUCCESS(PhGetProcessHeapFrontEndType(
                processHandle,
                heapInfo.BaseAddress,
                isWow64,
                &frontEndType
                )))
            {
                heapDebugInfo->Heaps[i].HeapFrontEndType = frontEndType;
            }

            NtClose(processHandle);
        }
    }

    if (HeapInformation)
        *HeapInformation = heapDebugInfo;
    else
        PhFree(heapDebugInfo);

    if (debugBuffer)
        RtlDestroyQueryDebugBuffer(debugBuffer);

    return STATUS_SUCCESS;
}

NTSTATUS PhQueryProcessLockInformation(
    _In_ HANDLE ProcessId,
    _Out_ PULONG NumberOfLocks,
    _Out_ PRTL_PROCESS_LOCK_INFORMATION* Locks
    )
{
    NTSTATUS status;
    PRTL_DEBUG_INFORMATION debugBuffer = NULL;
    PH_ARRAY array;

    for (ULONG i = 0x400000; ; i *= 2) // rev from Heap32First/Heap32Next (dmex)
    {
        if (!(debugBuffer = RtlCreateQueryDebugBuffer(i, FALSE)))
            return STATUS_UNSUCCESSFUL;

        status = RtlQueryProcessDebugInformation(
            ProcessId,
            RTL_QUERY_PROCESS_LOCKS,
            debugBuffer
            );

        if (!NT_SUCCESS(status))
        {
            RtlDestroyQueryDebugBuffer(debugBuffer);
            debugBuffer = NULL;
        }

        if (NT_SUCCESS(status) || status != STATUS_NO_MEMORY)
            break;

        if (2 * i <= i)
        {
            status = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    if (!NT_SUCCESS(status))
        return status;

    if (!debugBuffer->Locks)
    {
        // The RtlQueryProcessDebugInformation function has two bugs on some versions
        // when querying the ProcessId for a frozen (suspended) immersive process. (dmex)
        //
        // 1) It'll deadlock the current thread for 30 seconds.
        // 2) It'll return STATUS_SUCCESS but with a NULL Heaps buffer.
        //
        // A workaround was implemented using PhCreateExecutionRequiredRequest() (dmex)

        RtlDestroyQueryDebugBuffer(debugBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    PhInitializeArray(&array, sizeof(RTL_PROCESS_LOCK_INFORMATION), debugBuffer->Locks->NumberOfLocks);

    for (ULONG i = 0; i < debugBuffer->Locks->NumberOfLocks; i++)
    {
        PhAddItemArray(&array, &debugBuffer->Locks->Locks[i]);
    }

    *NumberOfLocks = (ULONG)PhFinalArrayCount(&array);
    *Locks = PhFinalArrayItems(&array);

    RtlDestroyQueryDebugBuffer(debugBuffer);

    return STATUS_SUCCESS;
}

// Queries if the specified architecture is supported on the current system,
// either natively or by any form of compatibility or emulation layer.
// rev from kernelbase!GetMachineTypeAttributes (dmex)
NTSTATUS PhGetMachineTypeAttributes(
    _In_ USHORT Machine,
    _Out_ MACHINE_ATTRIBUTES* Attributes
    )
{
    NTSTATUS status;
    HANDLE input[1] = { 0 };
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION output[6] = { 0 };
    ULONG returnLength;

    status = NtQuerySystemInformationEx(
        SystemSupportedProcessorArchitectures2,
        input,
        sizeof(input),
        output,
        sizeof(output),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        MACHINE_ATTRIBUTES attributes;

        memset(&attributes, 0, sizeof(MACHINE_ATTRIBUTES));

        for (ULONG i = 0; i < returnLength / sizeof(SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION); i++)
        {
            if (output[i].Machine == Machine)
            {
                if (output[i].KernelMode)
                    SetFlag(attributes, KernelEnabled);
                if (output[i].UserMode)
                    SetFlag(attributes, UserEnabled);
                if (output[i].WoW64Container)
                    SetFlag(attributes, Wow64Container);
                break;
            }
        }

        *Attributes = attributes;
    }

    return status;
}

BOOLEAN PhIsFirmwareSupported(
    VOID
    )
{
    UNICODE_STRING variableName = RTL_CONSTANT_STRING(L" ");
    ULONG variableValueLength = 0;
    GUID vendorGuid = { 0 };

    if (NtQuerySystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        NULL,
        &variableValueLength,
        NULL
        ) == STATUS_VARIABLE_NOT_FOUND)
    {
        return TRUE;
    }

    return FALSE;
}

// rev from GetFirmwareEnvironmentVariableW (dmex)
NTSTATUS PhGetFirmwareEnvironmentVariable(
    _In_ PCPH_STRINGREF VariableName,
    _In_ PCPH_STRINGREF VendorGuid,
    _Out_writes_bytes_opt_(*ValueLength) PVOID* ValueBuffer,
    _Out_opt_ PULONG ValueLength,
    _Out_opt_ PULONG ValueAttributes
    )
{
    NTSTATUS status;
    GUID vendorGuid;
    UNICODE_STRING variableName;
    PVOID valueBuffer;
    ULONG valueLength = 0;
    ULONG valueAttributes = 0;

    PhStringRefToUnicodeString(VariableName, &variableName);

    status = PhStringToGuid(
        VendorGuid,
        &vendorGuid
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtQuerySystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        NULL,
        &valueLength,
        &valueAttributes
        );

    if (status != STATUS_BUFFER_TOO_SMALL)
        return STATUS_UNSUCCESSFUL;

    valueBuffer = PhAllocate(valueLength);
    memset(valueBuffer, 0, valueLength);

    status = NtQuerySystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        valueBuffer,
        &valueLength,
        &valueAttributes
        );

    if (NT_SUCCESS(status))
    {
        if (ValueBuffer)
            *ValueBuffer = valueBuffer;
        else
            PhFree(valueBuffer);

        if (ValueLength)
            *ValueLength = valueLength;

        if (ValueAttributes)
            *ValueAttributes = valueAttributes;
    }
    else
    {
        PhFree(valueBuffer);
    }

    return status;
}

NTSTATUS PhSetFirmwareEnvironmentVariable(
    _In_ PCPH_STRINGREF VariableName,
    _In_ PCPH_STRINGREF VendorGuid,
    _In_reads_bytes_opt_(ValueLength) PVOID ValueBuffer,
    _In_ ULONG ValueLength,
    _In_ ULONG Attributes
    )
{
    NTSTATUS status;
    GUID vendorGuid;
    UNICODE_STRING variableName;

    PhStringRefToUnicodeString(VariableName, &variableName);

    status = PhStringToGuid(
        VendorGuid,
        &vendorGuid
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtSetSystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        ValueBuffer,
        ValueLength,
        Attributes
        );

    return status;
}

NTSTATUS PhEnumFirmwareEnvironmentValues(
    _In_ SYSTEM_ENVIRONMENT_INFORMATION_CLASS InformationClass,
    _Out_ PVOID* Variables
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferLength;

    bufferLength = PAGE_SIZE;
    buffer = PhAllocate(bufferLength);

    while (TRUE)
    {
        status = NtEnumerateSystemEnvironmentValuesEx(
            InformationClass,
            buffer,
            &bufferLength
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferLength);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Variables = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhSetSystemEnvironmentBootToFirmware(
    VOID
    )
{
    static CONST GUID EFI_GLOBAL_VARIABLE_GUID = { 0x8be4df61, 0x93ca, 0x11d2, { 0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c } };
    static CONST UNICODE_STRING OsIndicationsSupportedName = RTL_CONSTANT_STRING(L"OsIndicationsSupported");
    static CONST UNICODE_STRING OsIndicationsName = RTL_CONSTANT_STRING(L"OsIndications");
    const ULONG64 EFI_OS_INDICATIONS_BOOT_TO_FW_UI = 0x0000000000000001ULL;
    ULONG osIndicationsLength = sizeof(ULONG64);
    ULONG osIndicationsAttributes = 0;
    ULONG64 osIndicationsSupported = 0;
    ULONG64 osIndicationsValue = 0;
    NTSTATUS status;

    status = NtQuerySystemEnvironmentValueEx(
        &OsIndicationsSupportedName,
        &EFI_GLOBAL_VARIABLE_GUID,
        &osIndicationsSupported,
        &osIndicationsLength,
        NULL
        );

    if (status == STATUS_VARIABLE_NOT_FOUND || !(osIndicationsSupported & EFI_OS_INDICATIONS_BOOT_TO_FW_UI))
    {
        status = STATUS_NOT_SUPPORTED;
    }

    if (NT_SUCCESS(status))
    {
        status = NtQuerySystemEnvironmentValueEx(
            &OsIndicationsName,
            &EFI_GLOBAL_VARIABLE_GUID,
            &osIndicationsValue,
            &osIndicationsLength,
            &osIndicationsAttributes
            );

        if (NT_SUCCESS(status) || status == STATUS_VARIABLE_NOT_FOUND)
        {
            osIndicationsValue |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;

            if (status == STATUS_VARIABLE_NOT_FOUND)
            {
                osIndicationsAttributes = EFI_VARIABLE_NON_VOLATILE;
            }

            status = NtSetSystemEnvironmentValueEx(
                &OsIndicationsName,
                &EFI_GLOBAL_VARIABLE_GUID,
                &osIndicationsValue,
                osIndicationsLength,
                osIndicationsAttributes
                );
        }
    }

    return status;
}

// rev from RtlpCreateExecutionRequiredRequest (dmex)
/**
 * Create a new power request object. The process will continue to run instead of being suspended or terminated by PLM (Process Lifetime Management).
 * This is mandatory on Windows 8 and above to prevent threads created by DebugActiveProcess, QueueUserAPC and RtlQueryProcessDebug* functions from deadlocking the current application.
 *
 * \param ProcessHandle A handle to the process for which the power request is to be created.
 * \param PowerRequestHandle A pointer to a variable that receives a handle to the new power request.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhCreateExecutionRequiredRequest(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE PowerRequestHandle
    )
{
    NTSTATUS status;
    HANDLE powerRequestHandle = NULL;
    COUNTED_REASON_CONTEXT powerRequestReason;
    POWER_REQUEST_ACTION powerRequestAction;

    memset(&powerRequestReason, 0, sizeof(COUNTED_REASON_CONTEXT));
    powerRequestReason.Version = POWER_REQUEST_CONTEXT_VERSION;
    powerRequestReason.Flags = POWER_REQUEST_CONTEXT_NOT_SPECIFIED;

    status = NtPowerInformation(
        PlmPowerRequestCreate,
        &powerRequestReason,
        sizeof(COUNTED_REASON_CONTEXT),
        &powerRequestHandle,
        sizeof(HANDLE)
        );

    if (!NT_SUCCESS(status))
        return status;

    memset(&powerRequestAction, 0, sizeof(POWER_REQUEST_ACTION));
    powerRequestAction.PowerRequestHandle = powerRequestHandle;
    powerRequestAction.RequestType = PowerRequestExecutionRequiredInternal;
    powerRequestAction.SetAction = TRUE;
    powerRequestAction.ProcessHandle = ProcessHandle;

    status = NtPowerInformation(
        PowerRequestAction,
        &powerRequestAction,
        sizeof(POWER_REQUEST_ACTION),
        NULL,
        0
        );

    if (NT_SUCCESS(status))
    {
        *PowerRequestHandle = powerRequestHandle;
    }
    else
    {
        NtClose(powerRequestHandle);
    }

    return status;
}

// rev from RtlpDestroyExecutionRequiredRequest (dmex)
NTSTATUS PhDestroyExecutionRequiredRequest(
    _In_opt_ _Post_ptr_invalid_ HANDLE PowerRequestHandle
    )
{
    POWER_REQUEST_ACTION requestPowerAction;

    if (!PowerRequestHandle)
        return STATUS_INVALID_PARAMETER;
    memset(&requestPowerAction, 0, sizeof(POWER_REQUEST_ACTION));
    requestPowerAction.PowerRequestHandle = PowerRequestHandle;
    requestPowerAction.RequestType = PowerRequestExecutionRequiredInternal;
    requestPowerAction.SetAction = FALSE;
    requestPowerAction.ProcessHandle = NULL;

    NtPowerInformation(
        PowerRequestAction,
        &requestPowerAction,
        sizeof(POWER_REQUEST_ACTION),
        NULL,
        0
        );

    return NtClose(PowerRequestHandle);
}

NTSTATUS PhGetProcessorNominalFrequency(
    _In_ PH_PROCESSOR_NUMBER ProcessorNumber,
    _Out_ PULONG NominalFrequency
    )
{
    NTSTATUS status;
    POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT frequencyInput;
    POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT frequencyOutput;

    memset(&frequencyInput, 0, sizeof(frequencyInput));
    frequencyInput.InternalType = PowerInternalProcessorBrandedFrequency;
    frequencyInput.ProcessorNumber.Group = ProcessorNumber.Group; // USHRT_MAX for max
    frequencyInput.ProcessorNumber.Number = (BYTE)ProcessorNumber.Number; // UCHAR_MAX for max
    frequencyInput.ProcessorNumber.Reserved = 0; // UCHAR_MAX

    memset(&frequencyOutput, 0, sizeof(frequencyOutput));
    frequencyOutput.Version = POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION;

    status = NtPowerInformation(
        PowerInformationInternal,
        &frequencyInput,
        sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT),
        &frequencyOutput,
        sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT)
        );

    if (NT_SUCCESS(status))
    {
        if (frequencyOutput.Version == POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION)
        {
            *NominalFrequency = frequencyOutput.NominalFrequency;
        }
        else
        {
            status = STATUS_INVALID_KERNEL_INFO_VERSION;
        }
    }

    return status;
}

//
// Process freeze/thaw support
//

NTSTATUS PhFreezeProcess(
    _Out_ PHANDLE FreezeHandle,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE processHandle;
    HANDLE stateHandle;

    if (!(NtCreateProcessStateChange_Import() && NtChangeProcessState_Import()))
        return STATUS_UNSUCCESSFUL;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME,
        ProcessId
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    status = NtCreateProcessStateChange_Import()(
        &stateHandle,
        STATECHANGE_SET_ATTRIBUTES,
        &objectAttributes,
        processHandle,
        0
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(processHandle);
        return status;
    }

    status = NtChangeProcessState_Import()(
        stateHandle,
        processHandle,
        ProcessStateChangeSuspend,
        NULL,
        0,
        0
        );

    if (NT_SUCCESS(status))
    {
        *FreezeHandle = stateHandle;
    }

    NtClose(processHandle);

    return status;
}

NTSTATUS PhThawProcess(
    _In_ HANDLE FreezeHandle,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (!(NtCreateProcessStateChange_Import() && NtChangeProcessState_Import()))
        return STATUS_UNSUCCESSFUL;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME,
        ProcessId
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtChangeProcessState_Import()(
        FreezeHandle,
        processHandle,
        ProcessStateChangeResume,
        NULL,
        0,
        0
        );

    NtClose(processHandle);

    return status;
}

NTSTATUS PhFreezeThread(
    _Out_ PHANDLE FreezeHandle,
    _In_ HANDLE ThreadId
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE threadHandle;
    HANDLE stateHandle;

    if (!(NtCreateThreadStateChange_Import() && NtChangeThreadState_Import()))
        return STATUS_UNSUCCESSFUL;

    status = PhOpenThread(
        &threadHandle,
        PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME,
        ThreadId
        );

    if (!NT_SUCCESS(status))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    status = NtCreateThreadStateChange_Import()(
        &stateHandle,
        STATECHANGE_SET_ATTRIBUTES,
        &objectAttributes,
        threadHandle,
        0
        );

    if (!NT_SUCCESS(status))
    {
        NtClose(threadHandle);
        return status;
    }

    status = NtChangeThreadState_Import()(
        stateHandle,
        threadHandle,
        ThreadStateChangeResume,
        NULL,
        0,
        0
        );

    if (NT_SUCCESS(status))
    {
        *FreezeHandle = stateHandle;
    }

    NtClose(threadHandle);

    return status;
}

NTSTATUS PhThawThread(
    _In_ HANDLE FreezeHandle,
    _In_ HANDLE ThreadId
    )
{
    NTSTATUS status;
    HANDLE threadHandle;

    if (!(NtCreateThreadStateChange_Import() && NtChangeThreadState_Import()))
        return STATUS_UNSUCCESSFUL;

    status = PhOpenThread(
        &threadHandle,
        THREAD_SET_INFORMATION | THREAD_SUSPEND_RESUME,
        ThreadId
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtChangeThreadState_Import()(
        FreezeHandle,
        threadHandle,
        ThreadStateChangeResume,
        NULL,
        0,
        0
        );

    NtClose(threadHandle);

    return status;
}

//
// Process execution request support
//

static PH_INITONCE PhExecutionRequestInitOnce = PH_INITONCE_INIT;
static PPH_HASHTABLE PhExecutionRequestHashtable = NULL;

typedef struct _PH_EXECUTIONREQUEST_CACHE_ENTRY
{
    HANDLE ProcessId;
    HANDLE ExecutionRequestHandle;
} PH_EXECUTIONREQUEST_CACHE_ENTRY, *PPH_EXECUTIONREQUEST_CACHE_ENTRY;

static BOOLEAN NTAPI PhExecutionRequestHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return
        ((PPH_EXECUTIONREQUEST_CACHE_ENTRY)Entry1)->ProcessId ==
        ((PPH_EXECUTIONREQUEST_CACHE_ENTRY)Entry2)->ProcessId;
}

static ULONG NTAPI PhExecutionRequestHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return HandleToUlong(((PPH_EXECUTIONREQUEST_CACHE_ENTRY)Entry)->ProcessId) / 4;
}

BOOLEAN PhInitializeExecutionRequestTable(
    VOID
    )
{
    if (PhBeginInitOnce(&PhExecutionRequestInitOnce))
    {
        PhExecutionRequestHashtable = PhCreateHashtable(
            sizeof(PH_EXECUTIONREQUEST_CACHE_ENTRY),
            PhExecutionRequestHashtableEqualFunction,
            PhExecutionRequestHashtableHashFunction,
            1
            );

        PhEndInitOnce(&PhExecutionRequestInitOnce);
    }

    return TRUE;
}

BOOLEAN PhIsProcessExecutionRequired(
    _In_ HANDLE ProcessId
    )
{
    if (PhInitializeExecutionRequestTable())
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        entry.ProcessId = ProcessId;

        if (PhFindEntryHashtable(PhExecutionRequestHashtable, &entry))
        {
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS PhProcessExecutionRequiredEnable(
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE requestHandle;

    if (PhInitializeExecutionRequestTable())
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        entry.ProcessId = ProcessId;

        if (PhFindEntryHashtable(PhExecutionRequestHashtable, &entry))
        {
            return STATUS_SUCCESS;
        }
    }

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_LIMITED_INFORMATION,
        ProcessId
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhCreateExecutionRequiredRequest(
        processHandle,
        &requestHandle
        );

    NtClose(processHandle);

    if (NT_SUCCESS(status))
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        entry.ProcessId = ProcessId;
        entry.ExecutionRequestHandle = requestHandle;

        PhAddEntryHashtable(PhExecutionRequestHashtable, &entry);
    }

    return status;
}

NTSTATUS PhProcessExecutionRequiredDisable(
    _In_ HANDLE ProcessId
    )
{
    if (PhInitializeExecutionRequestTable())
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY lookupEntry;
        PPH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        lookupEntry.ProcessId = ProcessId;

        if (entry = PhFindEntryHashtable(PhExecutionRequestHashtable, &lookupEntry))
        {
            HANDLE requestHandle = entry->ExecutionRequestHandle;

            PhRemoveEntryHashtable(PhExecutionRequestHashtable, &lookupEntry);

            if (requestHandle)
            {
                return PhDestroyExecutionRequiredRequest(requestHandle);
            }
        }
    }

    return STATUS_UNSUCCESSFUL;
}

// KnownDLLs cache support

static PH_INITONCE PhKnownDllsInitOnce = PH_INITONCE_INIT;
static PPH_HASHTABLE PhKnownDllsHashtable = NULL;

typedef struct _PH_KNOWNDLL_CACHE_ENTRY
{
    PPH_STRING FileName;
} PH_KNOWNDLL_CACHE_ENTRY, *PPH_KNOWNDLL_CACHE_ENTRY;

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
static BOOLEAN NTAPI PhKnownDllsHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return PhEqualStringRef(&((PPH_KNOWNDLL_CACHE_ENTRY)Entry1)->FileName->sr, &((PPH_KNOWNDLL_CACHE_ENTRY)Entry2)->FileName->sr, FALSE);
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
static ULONG NTAPI PhKnownDllsHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRefEx(&((PPH_KNOWNDLL_CACHE_ENTRY)Entry)->FileName->sr, FALSE, PH_STRING_HASH_XXH32);
}

_Function_class_(PH_ENUM_DIRECTORY_OBJECTS)
static BOOLEAN NTAPI PhpKnownDllObjectsCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    PVOID viewBase;
    SIZE_T viewSize;
    PPH_STRING fileName;

    if (!PhStringRefToUnicodeString(Name, &objectName))
        return TRUE;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenSection(
        &sectionHandle,
        SECTION_MAP_READ,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        return TRUE;

    viewSize = PAGE_SIZE;
    viewBase = NULL;

    status = NtMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &viewBase,
        0,
        0,
        NULL,
        &viewSize,
        ViewUnmap,
        WindowsVersion < WINDOWS_10_RS2 ? 0 : MEM_MAPPED,
        PAGE_READONLY
        );

    NtClose(sectionHandle);

    if (!NT_SUCCESS(status))
        return TRUE;

    status = PhGetProcessMappedFileName(
        NtCurrentProcess(),
        viewBase,
        &fileName
        );

    NtUnmapViewOfSection(NtCurrentProcess(), viewBase);

    if (NT_SUCCESS(status))
    {
        PH_KNOWNDLL_CACHE_ENTRY entry;

        entry.FileName = fileName;

        PhAddEntryHashtable(PhKnownDllsHashtable, &entry);
    }

    return TRUE;
}

VOID PhInitializeKnownDlls(
    _In_ PCWSTR ObjectName
    )
{
    UNICODE_STRING directoryName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE directoryHandle;

    RtlInitUnicodeString(&directoryName, ObjectName);
    InitializeObjectAttributes(
        &objectAttributes,
        &directoryName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    if (NT_SUCCESS(NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        )))
    {
        PhEnumDirectoryObjects(
            directoryHandle,
            PhpKnownDllObjectsCallback,
            NULL
            );
        NtClose(directoryHandle);
    }
}

BOOLEAN PhInitializeKnownDllsTable(
    VOID
    )
{
    if (PhBeginInitOnce(&PhKnownDllsInitOnce))
    {
        PhKnownDllsHashtable = PhCreateHashtable(
            sizeof(PH_KNOWNDLL_CACHE_ENTRY),
            PhKnownDllsHashtableEqualFunction,
            PhKnownDllsHashtableHashFunction,
            10
            );

        PhInitializeKnownDlls(L"\\KnownDlls");
        PhInitializeKnownDlls(L"\\KnownDlls32");
#ifdef _ARM64_
        PhInitializeKnownDlls(L"\\KnownDllsArm32");
        PhInitializeKnownDlls(L"\\KnownDllsChpe32");
#endif
        PhEndInitOnce(&PhKnownDllsInitOnce);
    }

    return TRUE;
}

BOOLEAN PhIsKnownDllFileName(
    _In_ PPH_STRING FileName
    )
{
    if (PhInitializeKnownDllsTable())
    {
        PH_KNOWNDLL_CACHE_ENTRY entry;

        entry.FileName = FileName;

        if (PhFindEntryHashtable(PhKnownDllsHashtable, &entry))
        {
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS PhGetSystemLogicalProcessorInformation(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _Out_ PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *Buffer,
    _Out_ PULONG BufferLength
    )
{
    static ULONG initialBufferSize[] = { 0x200, 0x80, 0x100, 0x1000 };
    NTSTATUS status;
    ULONG classIndex;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    switch (RelationshipType)
    {
    case RelationProcessorCore:
        classIndex = 0;
        break;
    case RelationProcessorPackage:
        classIndex = 1;
        break;
    case RelationGroup:
        classIndex = 2;
        break;
    case RelationAll:
        classIndex = 3;
        break;
    default:
        return STATUS_INVALID_INFO_CLASS;
    }

    bufferSize = initialBufferSize[classIndex];
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformationEx(
        SystemLogicalProcessorAndGroupInformation,
        &RelationshipType,
        sizeof(LOGICAL_PROCESSOR_RELATIONSHIP),
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformationEx(
            SystemLogicalProcessorAndGroupInformation,
            &RelationshipType,
            sizeof(LOGICAL_PROCESSOR_RELATIONSHIP),
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x100000) initialBufferSize[classIndex] = bufferSize;
    *Buffer = buffer;
    *BufferLength = bufferSize;

    return status;
}

NTSTATUS PhGetSystemLogicalProcessorRelationInformation(
    _Out_ PPH_LOGICAL_PROCESSOR_INFORMATION LogicalProcessorInformation
    )
{
    NTSTATUS status;
    ULONG logicalInformationLength = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalInformation;

    status = PhGetSystemLogicalProcessorInformation(
        RelationAll,
        &logicalInformation,
        &logicalInformationLength
        );

    if (NT_SUCCESS(status))
    {
        ULONG processorCoreCount = 0;
        ULONG processorNumaCount = 0;
        ULONG processorLogicalCount = 0;
        ULONG processorPackageCount = 0;

        for (
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo = logicalInformation;
            (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, logicalInformationLength);
            processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
            )
        {
            switch (processorInfo->Relationship)
            {
            case RelationProcessorCore:
                {
                    processorCoreCount++;

                    for (USHORT j = 0; j < processorInfo->Processor.GroupCount; j++)
                    {
                        processorLogicalCount += PhCountBitsUlongPtr(processorInfo->Processor.GroupMask[j].Mask); // RtlNumberOfSetBitsUlongPtr
                    }
                }
                break;
            case RelationNumaNode:
                processorNumaCount++;
                break;
            case RelationProcessorPackage:
                processorPackageCount++;
                break;
            }
        }

        memset(LogicalProcessorInformation, 0, sizeof(PH_LOGICAL_PROCESSOR_INFORMATION));
        LogicalProcessorInformation->ProcessorCoreCount = processorCoreCount;
        LogicalProcessorInformation->ProcessorNumaCount = processorNumaCount;
        LogicalProcessorInformation->ProcessorLogicalCount = processorLogicalCount;
        LogicalProcessorInformation->ProcessorPackageCount = processorPackageCount;

        PhFree(logicalInformation);
    }

    return status;
}

// based on RtlIsProcessorFeaturePresent (dmex)
BOOLEAN PhIsProcessorFeaturePresent(
    _In_ ULONG ProcessorFeature
    )
{
    if (WindowsVersion < WINDOWS_NEW && ProcessorFeature < PROCESSOR_FEATURE_MAX)
    {
        return USER_SHARED_DATA->ProcessorFeatures[ProcessorFeature];
    }

    return !!IsProcessorFeaturePresent(ProcessorFeature); // RtlIsProcessorFeaturePresent
}

VOID PhGetCurrentProcessorNumber(
    _Out_ PPROCESSOR_NUMBER ProcessorNumber
    )
{
    //if (PhIsProcessorFeaturePresent(PF_RDPID_INSTRUCTION_AVAILABLE))
//    _rdpid_u32();
//if (PhIsProcessorFeaturePresent(PF_RDTSCP_INSTRUCTION_AVAILABLE))
//    __rdtscp();

    memset(ProcessorNumber, 0, sizeof(PROCESSOR_NUMBER));

    RtlGetCurrentProcessorNumberEx(ProcessorNumber);
}

// based on GetActiveProcessorCount (dmex)
USHORT PhGetActiveProcessorCount(
    _In_ USHORT ProcessorGroup
    )
{
    if (PhSystemProcessorInformation.ActiveProcessorCount)
    {
        USHORT numberOfProcessors = 0;

        if (ProcessorGroup == ALL_PROCESSOR_GROUPS)
        {
            for (USHORT i = 0; i < PhSystemProcessorInformation.NumberOfProcessorGroups; i++)
            {
                numberOfProcessors += PhSystemProcessorInformation.ActiveProcessorCount[i];
            }
        }
        else
        {
            if (ProcessorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups)
            {
                numberOfProcessors = PhSystemProcessorInformation.ActiveProcessorCount[ProcessorGroup];
            }
        }

        return numberOfProcessors;
    }
    else
    {
        return PhSystemProcessorInformation.NumberOfProcessors;
    }
}

NTSTATUS PhGetProcessorNumberFromIndex(
    _In_ ULONG ProcessorIndex,
    _Out_ PPH_PROCESSOR_NUMBER ProcessorNumber
    )
{
    USHORT processorIndex = 0;

    for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
    {
        USHORT processorCount = PhGetActiveProcessorCount(processorGroup);

        for (USHORT processorNumber = 0; processorNumber < processorCount; processorNumber++)
        {
            if (processorIndex++ == ProcessorIndex)
            {
                memset(ProcessorNumber, 0, sizeof(PH_PROCESSOR_NUMBER));
                ProcessorNumber->Group = processorGroup;
                ProcessorNumber->Number = processorNumber;
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS PhGetProcessorGroupActiveAffinityMask(
    _In_ USHORT ProcessorGroup,
    _Out_ PKAFFINITY ActiveProcessorMask
    )
{
    NTSTATUS status;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInformation;
    ULONG processorInformationLength;

    status = PhGetSystemLogicalProcessorInformation(
        RelationGroup,
        &processorInformation,
        &processorInformationLength
        );

    if (NT_SUCCESS(status))
    {
        if (ProcessorGroup < processorInformation->Group.ActiveGroupCount)
        {
            *ActiveProcessorMask = processorInformation->Group.GroupInfo[ProcessorGroup].ActiveProcessorMask;
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }

        PhFree(processorInformation);
    }

    return status;
}

NTSTATUS PhGetProcessorSystemAffinityMask(
    _Out_ PKAFFINITY ActiveProcessorsAffinityMask
    )
{
    if (PhSystemProcessorInformation.SingleProcessorGroup)
    {
        *ActiveProcessorsAffinityMask = PhSystemBasicInformation.ActiveProcessorsAffinityMask;
        return STATUS_SUCCESS;
    }
    else
    {
        PROCESSOR_NUMBER processorNumber;

        PhGetCurrentProcessorNumber(&processorNumber);

        return PhGetProcessorGroupActiveAffinityMask(processorNumber.Group, ActiveProcessorsAffinityMask);
    }
}

// rev from GetNumaHighestNodeNumber (dmex)
NTSTATUS PhGetNumaHighestNodeNumber(
    _Out_ PUSHORT NodeNumber
    )
{
    NTSTATUS status;
    SYSTEM_NUMA_INFORMATION numaProcessorMap;

    status = NtQuerySystemInformation(
        SystemNumaProcessorMap,
        &numaProcessorMap,
        sizeof(SYSTEM_NUMA_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *NodeNumber = (USHORT)numaProcessorMap.HighestNodeNumber;
    }

    return status;
}

// rev from GetNumaProcessorNodeEx (dmex)
BOOLEAN PhGetNumaProcessorNode(
    _In_ PPH_PROCESSOR_NUMBER ProcessorNumber,
    _Out_ PUSHORT NodeNumber
    )
{
    NTSTATUS status;
    SYSTEM_NUMA_INFORMATION numaProcessorMap;
    USHORT processorNode = 0;

    if (ProcessorNumber->Group >= 20 || ProcessorNumber->Number >= MAXIMUM_PROC_PER_GROUP)
    {
        *NodeNumber = USHRT_MAX;
        return FALSE;
    }

    status = NtQuerySystemInformation(
        SystemNumaProcessorMap,
        &numaProcessorMap,
        sizeof(SYSTEM_NUMA_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        *NodeNumber = USHRT_MAX;
        return FALSE;
    }

    while (
        numaProcessorMap.ActiveProcessorsGroupAffinity[processorNode].Group != ProcessorNumber->Group ||
        (numaProcessorMap.ActiveProcessorsGroupAffinity[processorNode].Mask & AFFINITY_MASK(ProcessorNumber->Number)) == 0
        )
    {
        if (++processorNode > numaProcessorMap.HighestNodeNumber)
        {
            *NodeNumber = USHRT_MAX;
            return FALSE;
        }
    }

    *NodeNumber = processorNode;
    return TRUE;
}

// rev from GetNumaProximityNodeEx (dmex)
NTSTATUS PhGetNumaProximityNode(
    _In_ ULONG ProximityId,
    _Out_ PUSHORT NodeNumber
    )
{
    NTSTATUS status;
    SYSTEM_NUMA_PROXIMITY_MAP numaProximityMap;

    memset(&numaProximityMap, 0, sizeof(SYSTEM_NUMA_PROXIMITY_MAP));
    numaProximityMap.NodeProximityId = ProximityId;

    status = NtQuerySystemInformation(
        SystemNumaProximityNodeInformation,
        &numaProximityMap,
        sizeof(SYSTEM_NUMA_PROXIMITY_MAP),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *NodeNumber = numaProximityMap.NodeNumber;
    }

    return status;
}

NTSTATUS
DECLSPEC_GUARDNOCF
PhpSetInformationVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ VIRTUAL_MEMORY_INFORMATION_CLASS VmInformationClass,
    _In_ ULONG_PTR NumberOfEntries,
    _In_reads_(NumberOfEntries) PMEMORY_RANGE_ENTRY VirtualAddresses,
    _In_reads_bytes_(VmInformationLength) PVOID VmInformation,
    _In_ ULONG VmInformationLength
    )
{
    assert(NtSetInformationVirtualMemory_Import());

    return NtSetInformationVirtualMemory_Import()(
        ProcessHandle,
        VmInformationClass,
        NumberOfEntries,
        VirtualAddresses,
        VmInformation,
        VmInformationLength
        );
}

// rev from PrefetchVirtualMemory (dmex)
/**
 * Provides an efficient mechanism to bring into memory potentially discontiguous virtual address ranges in a process address space.
 *
 * \param ProcessHandle A handle to the process whose virtual address ranges are to be prefetched.
 * \param NumberOfEntries Number of entries in the array pointed to by the VirtualAddresses parameter.
 * \param VirtualAddresses A pointer to an array of MEMORY_RANGE_ENTRY structures which each specify a virtual address range
 * to be prefetched. The virtual address ranges may cover any part of the process address space accessible by the target process.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhPrefetchVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG_PTR NumberOfEntries,
    _In_ PMEMORY_RANGE_ENTRY VirtualAddresses
    )
{
    NTSTATUS status;
    ULONG prefetchInformationFlags;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&prefetchInformationFlags, 0, sizeof(prefetchInformationFlags));

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmPrefetchInformation,
        NumberOfEntries,
        VirtualAddresses,
        &prefetchInformationFlags,
        sizeof(prefetchInformationFlags)
        );

    return status;
}

// rev from OfferVirtualMemory (dmex)
//NTSTATUS PhOfferVirtualMemory(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID VirtualAddress,
//    _In_ SIZE_T NumberOfBytes,
//    _In_ MEMORY_PAGE_PRIORITY_INFORMATION Priority
//    )
//{
//    NTSTATUS status;
//    MEMORY_RANGE_ENTRY virtualMemoryRange;
//    ULONG virtualMemoryFlags;
//
//    if (!NtSetInformationVirtualMemory_Import())
//        return STATUS_PROCEDURE_NOT_FOUND;
//
//    // TODO: NtQueryVirtualMemory (dmex)
//
//    memset(&virtualMemoryRange, 0, sizeof(MEMORY_RANGE_ENTRY));
//    virtualMemoryRange.VirtualAddress = VirtualAddress;
//    virtualMemoryRange.NumberOfBytes = NumberOfBytes;
//
//    memset(&virtualMemoryFlags, 0, sizeof(virtualMemoryFlags));
//    virtualMemoryFlags = Priority;
//
//    status = PhpSetInformationVirtualMemory(
//        ProcessHandle,
//        VmPagePriorityInformation,
//        1,
//        &virtualMemoryRange,
//        &virtualMemoryFlags,
//        sizeof(virtualMemoryFlags)
//        );
//
//    return status;
//}
//
// rev from DiscardVirtualMemory (dmex)
//NTSTATUS PhDiscardVirtualMemory(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID VirtualAddress,
//    _In_ SIZE_T NumberOfBytes
//    )
//{
//    NTSTATUS status;
//    MEMORY_RANGE_ENTRY virtualMemoryRange;
//    ULONG virtualMemoryFlags;
//
//    if (!NtSetInformationVirtualMemory_Import())
//        return STATUS_PROCEDURE_NOT_FOUND;
//
//    memset(&virtualMemoryRange, 0, sizeof(MEMORY_RANGE_ENTRY));
//    virtualMemoryRange.VirtualAddress = VirtualAddress;
//    virtualMemoryRange.NumberOfBytes = NumberOfBytes;
//
//    memset(&virtualMemoryFlags, 0, sizeof(virtualMemoryFlags));
//
//    status = PhpSetInformationVirtualMemory(
//        ProcessHandle,
//        VmPagePriorityInformation,
//        1,
//        &virtualMemoryRange,
//        &virtualMemoryFlags,
//        sizeof(virtualMemoryFlags)
//        );
//
//    return status;
//}

NTSTATUS PhSetVirtualMemoryPagePriority(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG PagePriority,
    _In_ PVOID VirtualAddress,
    _In_ SIZE_T NumberOfBytes
    )
{
    NTSTATUS status;
    MEMORY_RANGE_ENTRY virtualMemoryRange;

    memset(&virtualMemoryRange, 0, sizeof(MEMORY_RANGE_ENTRY));
    virtualMemoryRange.VirtualAddress = VirtualAddress;
    virtualMemoryRange.NumberOfBytes = NumberOfBytes;

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmPagePriorityInformation,
        1,
        &virtualMemoryRange,
        &PagePriority,
        sizeof(PagePriority)
        );

    return status;
}

// rev from SetProcessValidCallTargets (dmex)
//NTSTATUS PhSetProcessValidCallTarget(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID VirtualAddress
//    )
//{
//    NTSTATUS status;
//    MEMORY_BASIC_INFORMATION basicInfo;
//    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
//    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
//    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
//    ULONG numberOfEntriesProcessed = 0;
//
//    if (!NtSetInformationVirtualMemory_Import())
//        return STATUS_PROCEDURE_NOT_FOUND;
//
//    status = NtQueryVirtualMemory(
//        ProcessHandle,
//        VirtualAddress,
//        MemoryBasicInformation,
//        &basicInfo,
//        sizeof(MEMORY_BASIC_INFORMATION),
//        NULL
//        );
//
//    if (!NT_SUCCESS(status))
//        return status;
//
//    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
//    cfgCallTargetInfo.Offset = (ULONG_PTR)VirtualAddress - (ULONG_PTR)basicInfo.AllocationBase;
//    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_VALID;
//
//    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
//    cfgCallTargetRangeInfo.VirtualAddress = basicInfo.AllocationBase;
//    cfgCallTargetRangeInfo.NumberOfBytes = basicInfo.RegionSize;
//
//    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
//    cfgCallTargetListInfo.NumberOfEntries = 1;
//    cfgCallTargetListInfo.Reserved = 0;
//    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
//    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;
//
//    status = PhpSetInformationVirtualMemory(
//        ProcessHandle,
//        VmCfgCallTargetInformation,
//        1,
//        &cfgCallTargetRangeInfo,
//        &cfgCallTargetListInfo,
//        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
//        );
//
//    if (status == STATUS_INVALID_PAGE_PROTECTION)
//        status = STATUS_SUCCESS;
//
//    return status;
//}

// rev from RtlGuardGrantSuppressedCallAccess (dmex)
NTSTATUS PhGuardGrantSuppressedCallAccess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    )
{
    NTSTATUS status;
    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
    ULONG numberOfEntriesProcessed = 0;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
    cfgCallTargetRangeInfo.VirtualAddress = PAGE_ALIGN(VirtualAddress);
    cfgCallTargetRangeInfo.NumberOfBytes = PAGE_SIZE;

    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
    cfgCallTargetInfo.Offset = BYTE_OFFSET(VirtualAddress);
    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_VALID;

    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
    cfgCallTargetListInfo.NumberOfEntries = 1;
    cfgCallTargetListInfo.Reserved = 0;
    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmCfgCallTargetInformation,
        1,
        &cfgCallTargetRangeInfo,
        &cfgCallTargetListInfo,
        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
        );

    if (status == STATUS_INVALID_PAGE_PROTECTION)
        status = STATUS_SUCCESS;

    return status;
}

// rev from RtlDisableXfgOnTarget (dmex)
NTSTATUS PhDisableXfgOnTarget(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    )
{
    NTSTATUS status;
    PS_SYSTEM_DLL_INIT_BLOCK systemDllInitBlock;
    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
    ULONG numberOfEntriesProcessed = 0;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    if (!NT_SUCCESS(status = PhGetSystemDllInitBlock(&systemDllInitBlock)))
        return status;

    // Check if CFG is disabled. PhGetProcessIsCFGuardEnabled(NtCurrentProcess());
    if (!(systemDllInitBlock.CfgBitMap && systemDllInitBlock.CfgBitMapSize))
        return STATUS_SUCCESS;

    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
    cfgCallTargetRangeInfo.VirtualAddress = PAGE_ALIGN(VirtualAddress);
    cfgCallTargetRangeInfo.NumberOfBytes = PAGE_SIZE;

    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
    cfgCallTargetInfo.Offset = BYTE_OFFSET(VirtualAddress);
    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_CONVERT_XFG_TO_CFG;

    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
    cfgCallTargetListInfo.NumberOfEntries = 1;
    cfgCallTargetListInfo.Reserved = 0;
    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmCfgCallTargetInformation,
        1,
        &cfgCallTargetRangeInfo,
        &cfgCallTargetListInfo,
        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
        );

    if (status == STATUS_INVALID_PAGE_PROTECTION)
        status = STATUS_SUCCESS;

    return status;
}

NTSTATUS PhGetSystemCompressionStoreInformation(
    _Out_ PPH_SYSTEM_STORE_COMPRESSION_INFORMATION SystemCompressionStoreInformation
    )
{
    NTSTATUS status;
    SYSTEM_STORE_INFORMATION storeInfo;
    SM_STORE_COMPRESSION_INFORMATION_REQUEST compressionInfo;

    memset(&compressionInfo, 0, sizeof(SM_STORE_COMPRESSION_INFORMATION_REQUEST));
    compressionInfo.Version = SYSTEM_STORE_COMPRESSION_INFORMATION_VERSION_V1;

    memset(&storeInfo, 0, sizeof(SYSTEM_STORE_INFORMATION));
    storeInfo.Version = SYSTEM_STORE_INFORMATION_VERSION;
    storeInfo.StoreInformationClass = MemCompressionInfoRequest;
    storeInfo.Data = &compressionInfo;
    storeInfo.Length = SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE_V1;

    status = NtQuerySystemInformation(
        SystemStoreInformation,
        &storeInfo,
        sizeof(SYSTEM_STORE_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        memset(SystemCompressionStoreInformation, 0, sizeof(PH_SYSTEM_STORE_COMPRESSION_INFORMATION));
        SystemCompressionStoreInformation->CompressionPid = compressionInfo.CompressionPid;
        SystemCompressionStoreInformation->WorkingSetSize = compressionInfo.WorkingSetSize;
        SystemCompressionStoreInformation->TotalDataCompressed = compressionInfo.TotalDataCompressed;
        SystemCompressionStoreInformation->TotalCompressedSize = compressionInfo.TotalCompressedSize;
        SystemCompressionStoreInformation->TotalUniqueDataCompressed = compressionInfo.TotalUniqueDataCompressed;
    }

    return status;
}

// rev from PsmServiceExtHost!RmpMemoryMonitorEmptySystemStore (dmex)
NTSTATUS PhSystemCompressionStoreTrimRequest(
    VOID
    )
{
    NTSTATUS status;
    SYSTEM_STORE_INFORMATION storeInfo;
    SM_SYSTEM_STORE_TRIM_REQUEST trimRequestInfo;
    PH_SYSTEM_STORE_COMPRESSION_INFORMATION compressionInfo;

    status = PhGetSystemCompressionStoreInformation(&compressionInfo);

    if (!NT_SUCCESS(status))
        return status;

    memset(&trimRequestInfo, 0, sizeof(SM_SYSTEM_STORE_TRIM_REQUEST));
    trimRequestInfo.Version = SYSTEM_STORE_TRIM_INFORMATION_VERSION_V1;
    trimRequestInfo.PagesToTrim = BYTES_TO_PAGES(compressionInfo.WorkingSetSize);
    //trimRequestInfo.PartitionHandle = MEMORY_CURRENT_PARTITION_HANDLE;

    memset(&storeInfo, 0, sizeof(SYSTEM_STORE_INFORMATION));
    storeInfo.Version = SYSTEM_STORE_INFORMATION_VERSION;
    storeInfo.StoreInformationClass = SystemStoreTrimRequest;
    storeInfo.Data = &trimRequestInfo;
    storeInfo.Length = SYSTEM_STORE_TRIM_INFORMATION_SIZE_V1;

    status = NtQuerySystemInformation(
        SystemStoreInformation,
        &storeInfo,
        sizeof(SYSTEM_STORE_INFORMATION),
        NULL
        );

    return status;
}

NTSTATUS PhSystemCompressionStoreHighMemoryPriorityRequest(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN SetHighMemoryPriority
    )
{
    NTSTATUS status;
    SYSTEM_STORE_INFORMATION storeInfo;
    SM_STORE_HIGH_MEMORY_PRIORITY_REQUEST memoryPriorityInfo;

    memset(&memoryPriorityInfo, 0, sizeof(SM_STORE_HIGH_MEMORY_PRIORITY_REQUEST));
    memoryPriorityInfo.Version = SYSTEM_STORE_HIGH_MEM_PRIORITY_INFORMATION_VERSION;
    memoryPriorityInfo.SetHighMemoryPriority = SetHighMemoryPriority;
    memoryPriorityInfo.ProcessHandle = ProcessHandle;

    memset(&storeInfo, 0, sizeof(SYSTEM_STORE_INFORMATION));
    storeInfo.Version = SYSTEM_STORE_INFORMATION_VERSION;
    storeInfo.StoreInformationClass = StoreHighMemoryPriorityRequest;
    storeInfo.Data = &memoryPriorityInfo;
    storeInfo.Length = SYSTEM_STORE_TRIM_INFORMATION_SIZE_V1;

    status = NtQuerySystemInformation(
        SystemStoreInformation,
        &storeInfo,
        sizeof(SYSTEM_STORE_INFORMATION),
        NULL
        );

    return status;
}

NTSTATUS PhGetSystemFileCacheSize(
    _Out_ PSYSTEM_FILECACHE_INFORMATION CacheInfo
    )
{
    return NtQuerySystemInformation(
        SystemFileCacheInformationEx,
        CacheInfo,
        sizeof(SYSTEM_FILECACHE_INFORMATION),
        0
        );
}

// rev from SetSystemFileCacheSize (MSDN) (dmex)
NTSTATUS PhSetSystemFileCacheSize(
    _In_ SIZE_T MinimumFileCacheSize,
    _In_ SIZE_T MaximumFileCacheSize,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;
    SYSTEM_FILECACHE_INFORMATION cacheInfo;

    memset(&cacheInfo, 0, sizeof(SYSTEM_FILECACHE_INFORMATION));
    cacheInfo.MinimumWorkingSet = MinimumFileCacheSize;
    cacheInfo.MaximumWorkingSet = MaximumFileCacheSize;
    cacheInfo.Flags = Flags;

    status = NtSetSystemInformation(
        SystemFileCacheInformationEx,
        &cacheInfo,
        sizeof(SYSTEM_FILECACHE_INFORMATION)
        );

    return status;
}

NTSTATUS PhCreateEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ EVENT_TYPE EventType,
    _In_ BOOLEAN InitialState
    )
{
    NTSTATUS status;
    HANDLE eventHandle;
    OBJECT_ATTRIBUTES objectAttributes;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    status = NtCreateEvent(
        &eventHandle,
        DesiredAccess,
        &objectAttributes,
        EventType,
        InitialState
        );

    if (NT_SUCCESS(status))
    {
        *EventHandle = eventHandle;
    }

    return status;
}

// rev from DeviceIoControl (dmex)
NTSTATUS PhDeviceIoControlFile(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    if (DEVICE_TYPE_FROM_CTL_CODE(IoControlCode) == FILE_DEVICE_FILE_SYSTEM)
    {
        status = NtFsControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &ioStatusBlock,
            IoControlCode,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength
            );
    }
    else
    {
        status = NtDeviceIoControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &ioStatusBlock,
            IoControlCode,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength
            );
    }

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(DeviceHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
        {
            status = ioStatusBlock.Status;
        }
    }

    if (ReturnLength)
    {
        *ReturnLength = (ULONG)ioStatusBlock.Information;
    }

    return status;
}

// rev from RtlpWow64SelectSystem32PathInternal (dmex)
NTSTATUS PhWow64SelectSystem32Path(
    _In_ USHORT Machine,
    _In_ BOOLEAN IncludePathSeperator,
    _Out_ PPH_STRINGREF SystemPath
    )
{
    PWSTR WithSeperators;
    PWSTR WithoutSeperators;

    if (Machine != IMAGE_FILE_MACHINE_TARGET_HOST)
    {
        switch (Machine)
        {
        case IMAGE_FILE_MACHINE_I386:
            WithoutSeperators = L"SysWOW64";
            WithSeperators = L"\\SysWOW64\\";
            goto CreateResult;
        case IMAGE_FILE_MACHINE_ARMNT:
            WithoutSeperators = L"SysArm32";
            WithSeperators = L"\\SysArm32\\";
            goto CreateResult;
        case IMAGE_FILE_MACHINE_CHPE_X86:
            WithoutSeperators = L"SyChpe32";
            WithSeperators = L"\\SyChpe32\\";
            goto CreateResult;
        }

        if (Machine != IMAGE_FILE_MACHINE_AMD64 && Machine != IMAGE_FILE_MACHINE_ARM64)
            return STATUS_INVALID_PARAMETER;
    }

    WithSeperators = L"\\System32\\";
    WithoutSeperators = L"System32";

CreateResult:
    if (!IncludePathSeperator)
        WithSeperators = WithoutSeperators;

    PhInitializeStringRefLongHint(SystemPath, WithSeperators); // RtlInitUnicodeString
    return STATUS_SUCCESS;
}

/**
 * Retrieves information about a range of pages within the virtual address space of a specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Callback A callback function which is executed for each memory region.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_MEMORY_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;

    baseAddress = (PVOID)0;

    while (TRUE)
    {
        status = NtQueryVirtualMemory(
            ProcessHandle,
            baseAddress,
            MemoryBasicInformation,
            &basicInfo,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            );

        if (!NT_SUCCESS(status))
            break;

        if (basicInfo.State & MEM_FREE)
        {
            basicInfo.AllocationBase = basicInfo.BaseAddress;
            basicInfo.AllocationProtect = basicInfo.Protect;
        }

        status = Callback(ProcessHandle, &basicInfo, Context);

        if (!NT_SUCCESS(status))
            break;

        baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);

        if ((ULONG_PTR)baseAddress >= PhSystemBasicInformation.MaximumUserModeAddress)
            break;
    }

    return status;
}

/**
 * Retrieves information about a range of pages within the virtual address space of a specified process in batches for improved performance.
 *
 * \param ProcessHandle A handle to a process.
 * \param BaseAddress The base address at which to begin retrieving information.
 * \param BulkQuery A boolean indicating the mode of bulk query (accuracy vs reliability).
 * \param Callback A callback function which is executed for each memory region.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryBulk(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ BOOLEAN BulkQuery,
    _In_ PPH_ENUM_MEMORY_BULK_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;

    if (PhIsExecutingInWow64())
    {
        return STATUS_NOT_SUPPORTED;
    }

    if (!NtPssCaptureVaSpaceBulk_Import())
    {
        return STATUS_NOT_SUPPORTED;
    }

    // BulkQuery... TRUE:
    // * Faster.
    // * More accurate snapshots.
    // * Copies the entire VA space into local memory.
    // * Wastes large amounts of heap memory due to buffer doubling.
    // * Unsuitable for low-memory situations and fails with insufficient system resources.
    // * ...
    //
    // BulkQuery... FALSE:
    // * Slightly slower.
    // * Slightly less accurate snapshots.
    // * Does not copy the VA space.
    // * Does not waste heap memory.
    // * Suitable for low-memory situations and doesn't fail with insufficient system resources.
    // * ...

    if (BulkQuery)
    {
        SIZE_T bufferLength;
        PNTPSS_MEMORY_BULK_INFORMATION buffer;
        PMEMORY_BASIC_INFORMATION information;

        bufferLength = sizeof(NTPSS_MEMORY_BULK_INFORMATION) + sizeof(MEMORY_BASIC_INFORMATION[20]);
        buffer = PhAllocate(bufferLength);
        buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;

        // Allocate a large buffer and copy all entries.

        while ((status = NtPssCaptureVaSpaceBulk_Import()(
            ProcessHandle,
            BaseAddress,
            buffer,
            bufferLength,
            NULL
            )) == STATUS_MORE_ENTRIES)
        {
            PhFree(buffer);
            bufferLength *= 2;

            if (bufferLength > PH_LARGE_BUFFER_SIZE)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferLength);
            buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;
        }

        if (NT_SUCCESS(status))
        {
            // Skip the enumeration header.

            information = PTR_ADD_OFFSET(buffer, RTL_SIZEOF_THROUGH_FIELD(NTPSS_MEMORY_BULK_INFORMATION, NextValidAddress));

            // Execute the callback.

            Callback(ProcessHandle, information, buffer->NumberOfEntries, Context);
        }

        PhFree(buffer);
    }
    else
    {
        UCHAR stackBuffer[sizeof(NTPSS_MEMORY_BULK_INFORMATION) + sizeof(MEMORY_BASIC_INFORMATION[20])];
        SIZE_T bufferLength;
        PNTPSS_MEMORY_BULK_INFORMATION buffer;
        PMEMORY_BASIC_INFORMATION information;

        bufferLength = sizeof(stackBuffer);
        buffer = (PNTPSS_MEMORY_BULK_INFORMATION)stackBuffer;
        buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;
        buffer->NextValidAddress = BaseAddress;

        while (TRUE)
        {
            // Get a batch of entries.

            status = NtPssCaptureVaSpaceBulk_Import()(
                ProcessHandle,
                buffer->NextValidAddress,
                buffer,
                bufferLength,
                NULL
                );

            if (!NT_SUCCESS(status))
                break;

            // Skip the enumeration header.

            information = PTR_ADD_OFFSET(buffer, RTL_SIZEOF_THROUGH_FIELD(NTPSS_MEMORY_BULK_INFORMATION, NextValidAddress));

            // Execute the callback.

            if (!NT_SUCCESS(Callback(ProcessHandle, information, buffer->NumberOfEntries, Context)))
                break;

            // Get the next batch.

            if (status != STATUS_MORE_ENTRIES)
                break;
        }
    }

    return status;
}

/**
 * Retrieves information about the pages currently added to the working set of the specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Callback A callback function which is executed for each memory page.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryPages(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_MEMORY_PAGE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PMEMORY_WORKING_SET_INFORMATION pageInfo;

    status = PhGetProcessWorkingSetInformation(
        ProcessHandle,
        &pageInfo
        );

    if (NT_SUCCESS(status))
    {
        status = Callback(
            ProcessHandle,
            pageInfo->NumberOfEntries,
            pageInfo->WorkingSetInfo,
            Context
            );

        //for (ULONG_PTR i = 0; i < pageInfo->NumberOfEntries; i++)
        //{
        //    PMEMORY_WORKING_SET_BLOCK workingSetBlock = &pageInfo->WorkingSetInfo[i];
        //    PVOID virtualAddress = (PVOID)(workingSetBlock->VirtualPage << PAGE_SHIFT);
        //}

        PhFree(pageInfo);
    }

    return status;
}

/**
 * Retrieves extended information about the pages currently added to the working set at specific virtual addresses in the address space of the specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param BaseAddress The base address at which to begin retrieving information.
 * \param Size The total number of pages to query from the base address.
 * \param Callback A callback function which is executed for each memory page.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryAttributes(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size,
    _In_ PPH_ENUM_MEMORY_ATTRIBUTE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SIZE_T numberOfPages;
    ULONG_PTR virtualAddress;
    PMEMORY_WORKING_SET_EX_INFORMATION info;
    SIZE_T i;

    numberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, Size);
    virtualAddress = (ULONG_PTR)PAGE_ALIGN(BaseAddress);

    if (!numberOfPages)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    info = PhAllocatePage(numberOfPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION), NULL);

    if (!info)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    for (i = 0; i < numberOfPages; i++)
    {
        info[i].VirtualAddress = (PVOID)virtualAddress;
        virtualAddress += PAGE_SIZE;
    }

    status = NtQueryVirtualMemory(
        ProcessHandle,
        NULL,
        MemoryWorkingSetExInformation,
        info,
        numberOfPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        status = Callback(
            ProcessHandle,
            BaseAddress,
            Size,
            numberOfPages,
            info,
            Context
            );
    }

    PhFreePage(info);

CleanupExit:
    return status;
}

NTSTATUS PhGetKernelDebuggerInformation(
    _Out_opt_ PBOOLEAN KernelDebuggerEnabled,
    _Out_opt_ PBOOLEAN KernelDebuggerPresent
    )
{
    NTSTATUS status;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION debugInfo;

    status = NtQuerySystemInformation(
        SystemKernelDebuggerInformation,
        &debugInfo,
        sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (KernelDebuggerEnabled)
            *KernelDebuggerEnabled = debugInfo.KernelDebuggerEnabled;
        if (KernelDebuggerPresent)
            *KernelDebuggerPresent = !debugInfo.KernelDebuggerNotPresent;
    }

    return status;
}

// rev from BasepIsDebugPortPresent (dmex)
BOOLEAN PhIsDebugPortPresent(
    VOID
    )
{
    BOOLEAN isBeingDebugged;

    if (NT_SUCCESS(PhGetProcessIsBeingDebugged(NtCurrentProcess(), &isBeingDebugged)))
    {
        if (isBeingDebugged)
        {
            return TRUE;
        }
    }

    return FALSE;
}

// rev from IsDebuggerPresent (dmex)
/**
 * Determines whether the calling process is being debugged by a user-mode debugger.
 *
 * \return TRUE if the current process is running in the context of a debugger, otherwise the return value is FALSE.
 */
BOOLEAN PhIsDebuggerPresent(
    VOID
    )
{
#ifdef PHNT_NATIVE_DEBUGGER
    return !!IsDebuggerPresent();
#else
    return NtCurrentPeb()->BeingDebugged;
#endif
}

// rev from GetFileType (dmex)
/**
 * Retrieves the type of the specified file handle.
 *
 * \param ProcessHandle A handle to the process.
 * \param FileHandle A handle to the file.
 * \param DeviceType The type of the specified file
 *
 * \return Successful or errant status.
 */
NTSTATUS PhGetDeviceType(
    _In_opt_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _Out_ DEVICE_TYPE* DeviceType
    )
{
    NTSTATUS status;
    FILE_FS_DEVICE_INFORMATION debugInfo;
    IO_STATUS_BLOCK isb;

    status = PhQueryVolumeInformationFile(
        ProcessHandle,
        FileHandle,
        FileFsDeviceInformation,
        &debugInfo,
        sizeof(FILE_FS_DEVICE_INFORMATION),
        &isb
        );

    if (NT_SUCCESS(status))
    {
        *DeviceType = debugInfo.DeviceType;
    }

    return status;
}

BOOLEAN PhIsAppExecutionAliasTarget(
    _In_ PPH_STRING FileName
    )
{
    PPH_STRING targetFileName = NULL;
    PREPARSE_DATA_BUFFER reparseBuffer;
    ULONG reparseLength;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;

    if (PhIsNullOrEmptyString(FileName))
        return FALSE;

    if (!NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(FileName),
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT
        )))
    {
        return FALSE;
    }

    reparseLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    reparseBuffer = PhAllocateZero(reparseLength);

    if (NT_SUCCESS(NtFsControlFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_GET_REPARSE_POINT,
        NULL,
        0,
        reparseBuffer,
        reparseLength
        )))
    {
        if (
            IsReparseTagMicrosoft(reparseBuffer->ReparseTag) &&
            reparseBuffer->ReparseTag == IO_REPARSE_TAG_APPEXECLINK
            )
        {
            PCWSTR string;

            string = (PCWSTR)reparseBuffer->AppExecLinkReparseBuffer.StringList;

            for (ULONG i = 0; i < reparseBuffer->AppExecLinkReparseBuffer.StringCount; i++)
            {
                if (i == 2 && PhDoesFileExistWin32(string))
                {
                    targetFileName = PhCreateString(string);
                    break;
                }

                string += PhCountStringZ(string) + 1;
            }
        }
    }

    PhFree(reparseBuffer);
    NtClose(fileHandle);

    if (targetFileName)
    {
        if (PhDoesFileExistWin32(targetFileName->Buffer))
        {
            PhDereferenceObject(targetFileName);
            return TRUE;
        }

        PhDereferenceObject(targetFileName);
    }

    return FALSE;
}

NTSTATUS PhEnumProcessEnclaves(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID LdrEnclaveList,
    _In_ PPH_ENUM_PROCESS_ENCLAVES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    LIST_ENTRY enclaveList;
    LDR_SOFTWARE_ENCLAVE enclave;

    status = NtReadVirtualMemory(
        ProcessHandle,
        LdrEnclaveList,
        &enclaveList,
        sizeof(LIST_ENTRY),
        NULL
        );
    if (!NT_SUCCESS(status))
        return status;

    for (PLIST_ENTRY link = enclaveList.Flink;
         link != LdrEnclaveList;
         link = enclave.Links.Flink)
    {
        PVOID enclaveAddress;

        enclaveAddress = CONTAINING_RECORD(link, LDR_SOFTWARE_ENCLAVE, Links);

        status = NtReadVirtualMemory(
            ProcessHandle,
            link,
            &enclave,
            sizeof(enclave),
            NULL
            );
        if (!NT_SUCCESS(status))
            return status;

        if (!Callback(ProcessHandle, enclaveAddress, &enclave, Context))
            break;
    }

    return status;
}

#ifdef _M_ARM64
// rev from ntdll!RtlEcContextToNativeContext (jxy-s)
VOID PhEcContextToNativeContext(
    _Out_ PCONTEXT Context,
    _In_ PARM64EC_NT_CONTEXT EcContext
    )
{
    Context->ContextFlags = 0;

    //#define CONTEXT_AMD64   0x00100000L

    //#define CONTEXT_CONTROL         (CONTEXT_AMD64 | 0x00000001L)
    if (BooleanFlagOn(EcContext->ContextFlags, 0x00100000L | 0x00000001L))
        SetFlag(Context->ContextFlags, CONTEXT_CONTROL);

    //#define CONTEXT_INTEGER         (CONTEXT_AMD64 | 0x00000002L)
    if (BooleanFlagOn(EcContext->ContextFlags, 0x00100000L | 0x00000002L))
        SetFlag(Context->ContextFlags, CONTEXT_INTEGER);

    //#define CONTEXT_FLOATING_POINT  (CONTEXT_AMD64 | 0x00000008L)
    if (BooleanFlagOn(EcContext->ContextFlags, 0x00100000L | 0x00000008L))
        SetFlag(Context->ContextFlags, CONTEXT_FLOATING_POINT);

    SetFlag(Context->ContextFlags,
            EcContext->ContextFlags & (
                CONTEXT_EXCEPTION_ACTIVE |
                CONTEXT_SERVICE_ACTIVE |
                CONTEXT_EXCEPTION_REQUEST |
                CONTEXT_EXCEPTION_REPORTING
                ));

    Context->Cpsr = (EcContext->AMD64_EFlags & 0x00000100);         // Overflow Flag
    Context->Cpsr |= ((EcContext->AMD64_EFlags & 0x00000800) << 4); // Direction Flag
    Context->Cpsr |= ((EcContext->AMD64_EFlags & 0xFFFFFFC0) << 7); // Other Flags
    Context->Cpsr |= ((EcContext->AMD64_EFlags & 0x00000001) << 5); // Carry Flag
    Context->Cpsr <<= 13;

    Context->X0 = EcContext->X0;
    Context->X2 = EcContext->X2;
    Context->X4 = EcContext->X4;
    Context->X6 = EcContext->X6;
    Context->X7 = EcContext->X7;
    Context->X8 = EcContext->X8;
    Context->X9 = EcContext->X9;
    Context->X10 = EcContext->X10;
    Context->X11 = EcContext->X11;
    Context->X12 = EcContext->X12;
    Context->X14 = 0;
    Context->X15 = EcContext->X15;

    Context->X16 = EcContext->X16_0;
    Context->X16 |= ((ULONG64)EcContext->X16_1 << 16);
    Context->X16 |= ((ULONG64)EcContext->X16_2 << 32);
    Context->X16 |= ((ULONG64)EcContext->X16_3 << 48);

    Context->X17 = EcContext->X17_0;
    Context->X17 |= ((ULONG64)EcContext->X17_1 << 16);
    Context->X17 |= ((ULONG64)EcContext->X17_2 << 32);
    Context->X17 |= ((ULONG64)EcContext->X17_3 << 48);

    Context->X19 = EcContext->X19;
    Context->X21 = EcContext->X21;
    Context->X23 = 0;
    Context->X25 = EcContext->X25;
    Context->X27 = EcContext->X27;

    Context->Fp = EcContext->Fp;
    Context->Lr = EcContext->Lr;
    Context->Sp = EcContext->Sp;
    Context->Pc = EcContext->Pc;

    Context->V[0] = EcContext->V[0];
    Context->V[1] = EcContext->V[1];
    Context->V[2] = EcContext->V[2];
    Context->V[3] = EcContext->V[3];
    Context->V[4] = EcContext->V[4];
    Context->V[5] = EcContext->V[5];
    Context->V[6] = EcContext->V[6];
    Context->V[7] = EcContext->V[7];
    Context->V[8] = EcContext->V[8];
    Context->V[9] = EcContext->V[9];
    Context->V[10] = EcContext->V[10];
    Context->V[11] = EcContext->V[11];
    Context->V[12] = EcContext->V[12];
    Context->V[13] = EcContext->V[13];
    Context->V[14] = EcContext->V[14];
    Context->V[15] = EcContext->V[15];
    RtlZeroMemory(&Context->V[16], sizeof(ARM64_NT_NEON128));

    Context->Fpcr = (EcContext->AMD64_MxCsr & 0x00000080) == 0;         // IM: Invalid Operation Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000200) == 0) << 1; // ZM: Divide-by-Zero Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000400) == 0) << 2; // OM: Overflow Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000800) == 0) << 3; // UM: Underflow Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00001000) == 0) << 4; // PM: Precision Mask
    Context->Fpcr |= (EcContext->AMD64_MxCsr & 0x00000040) << 5;        // DAZ: Denormals Are Zero Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000100) == 0) << 7; // DM: Denormal Operation Mask
    Context->Fpcr |= (EcContext->AMD64_MxCsr & 0x00002000);             // FZ: Flush to Zero Mask
    Context->Fpcr |= (EcContext->AMD64_MxCsr & 0x0000C000);             // RC: Rounding Control
    Context->Fpcr <<= 8;

    Context->Fpsr = EcContext->AMD64_MxCsr & 1;            // IE: Invalid Operation Flag
    Context->Fpsr |= (EcContext->AMD64_MxCsr & 2) << 6;    // DE: Denormal Flag
    Context->Fpsr |= (EcContext->AMD64_MxCsr >> 1) & 0x1E; // ZE | OE | UE | PE: Zero, Overflow, Underflow, Precision Flags

    RtlZeroMemory(Context->Bcr, sizeof(Context->Bcr));
    RtlZeroMemory(Context->Bvr, sizeof(Context->Bvr));
    RtlZeroMemory(Context->Wcr, sizeof(Context->Wcr));
    RtlZeroMemory(Context->Wvr, sizeof(Context->Wvr));
}

// rev from ntdll!RtlNativeContextToEcContext (jxy-s)
VOID PhNativeContextToEcContext(
    _When_(InitializeEc, _Out_) _When_(!InitializeEc, _Inout_) PARM64EC_NT_CONTEXT EcContext,
    _In_ PCONTEXT Context,
    _In_ BOOLEAN InitializeEc
    )
{
    if (InitializeEc)
    {
        EcContext->ContextFlags = 0;

        //#define CONTEXT_AMD64   0x00100000L

        //#define CONTEXT_CONTROL         (CONTEXT_AMD64 | 0x00000001L)
        if (BooleanFlagOn(Context->ContextFlags, CONTEXT_CONTROL))
            SetFlag(EcContext->ContextFlags, (0x00100000L | 0x00000001L));

        //#define CONTEXT_INTEGER         (CONTEXT_AMD64 | 0x00000002L)
        if (BooleanFlagOn(Context->ContextFlags, CONTEXT_INTEGER))
            SetFlag(EcContext->ContextFlags, (0x00100000L | 0x00000002L));

        //#define CONTEXT_FLOATING_POINT  (CONTEXT_AMD64 | 0x00000008L)
        if (BooleanFlagOn(Context->ContextFlags, CONTEXT_FLOATING_POINT))
            SetFlag(EcContext->ContextFlags, (0x00100000L | 0x00000008L));

        EcContext->AMD64_P1Home = 0;
        EcContext->AMD64_P2Home = 0;
        EcContext->AMD64_P3Home = 0;
        EcContext->AMD64_P4Home = 0;
        EcContext->AMD64_P5Home = 0;
        EcContext->AMD64_P6Home = 0;

        EcContext->AMD64_Dr0 = 0;
        EcContext->AMD64_Dr1 = 0;
        EcContext->AMD64_Dr3 = 0;
        EcContext->AMD64_Dr6 = 0;
        EcContext->AMD64_Dr7 = 0;

        EcContext->AMD64_MxCsr_copy = 0;

        EcContext->AMD64_SegCs = 0x0033;
        EcContext->AMD64_SegDs = 0x002B;
        EcContext->AMD64_SegEs = 0x002B;
        EcContext->AMD64_SegFs = 0x0053;
        EcContext->AMD64_SegGs = 0x002B;
        EcContext->AMD64_SegSs = 0x002B;

        EcContext->AMD64_ControlWord = 0;
        EcContext->AMD64_StatusWord = 0;
        EcContext->AMD64_TagWord = 0;
        EcContext->AMD64_Reserved1 = 0;
        EcContext->AMD64_ErrorOpcode = 0;
        EcContext->AMD64_ErrorOffset = 0;
        EcContext->AMD64_ErrorSelector = 0;
        EcContext->AMD64_Reserved2= 0;
        EcContext->AMD64_DataOffset = 0;
        EcContext->AMD64_DataSelector = 0;
        EcContext->AMD64_Reserved3 = 0;

        EcContext->AMD64_MxCsr = 0;
        EcContext->AMD64_MxCsr_Mask = 0;

        EcContext->AMD64_St0_Reserved1 = 0;
        EcContext->AMD64_St0_Reserved2 = 0;
        EcContext->AMD64_St1_Reserved1 = 0;
        EcContext->AMD64_St1_Reserved2 = 0;
        EcContext->AMD64_St2_Reserved1 = 0;
        EcContext->AMD64_St2_Reserved2 = 0;
        EcContext->AMD64_St3_Reserved1 = 0;
        EcContext->AMD64_St3_Reserved2 = 0;
        EcContext->AMD64_St4_Reserved1 = 0;
        EcContext->AMD64_St4_Reserved2 = 0;
        EcContext->AMD64_St5_Reserved1 = 0;
        EcContext->AMD64_St5_Reserved2 = 0;
        EcContext->AMD64_St6_Reserved1 = 0;
        EcContext->AMD64_St6_Reserved2 = 0;
        EcContext->AMD64_St7_Reserved1 = 0;
        EcContext->AMD64_St7_Reserved2 = 0;

        EcContext->AMD64_EFlags = 0x202; // IF(EI) | RESERVED_1(always set)
    }

    EcContext->ContextFlags &= 0x7ffffffff;
    SetFlag(EcContext->ContextFlags,
            Context->ContextFlags & (
                CONTEXT_EXCEPTION_ACTIVE |
                CONTEXT_SERVICE_ACTIVE |
                CONTEXT_EXCEPTION_REQUEST |
                CONTEXT_EXCEPTION_REPORTING
                ));

    EcContext->AMD64_MxCsr_copy &= 0xFFFF0000;
    EcContext->AMD64_MxCsr_copy |= (Context->Fpsr & 0x00000080) >> 6;         // IM: Invalid Operation Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x00400000) >> 8;         // ZM: Divide-by-Zero Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x01000000) >> 9;         // OM: Overflow Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x00080000) >> 7;         // UM: Underflow Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x00800000) >> 7;         // PM: Precision Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpsr & 0x0000001E) << 1;         // Other Flags
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000100) == 0) << 7;  // DAZ: Denormals Are Zeros
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00008000) == 0) << 8;  // DM: Denormal Operation Mask
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000200) == 0) << 9;  // FZ: Flush to Zero
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000400) == 0) << 10; // RC: Rounding Control
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000800) == 0) << 11; // RC: Rounding Control
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00001000) == 0) << 12; // RC: Rounding Control
    EcContext->AMD64_MxCsr_copy |= Context->Fpsr & 0x00000001;

    EcContext->AMD64_EFlags &= 0xFFFFF63E;
    EcContext->AMD64_EFlags |= ((Context->Cpsr & 0x200000) >> 13);         // Overflow Flag
    EcContext->AMD64_EFlags |= ((Context->Cpsr & 0x10000000) >> 17);       // Direction Flag
    EcContext->AMD64_EFlags |= (((Context->Cpsr >> 5) & 0x1000000) >> 20); // Carry Flag
    EcContext->AMD64_EFlags |= ((Context->Cpsr & 0xC0FFFFFF) >> 20);       // Other Flags

    EcContext->Pc = Context->Pc;

    EcContext->X8 = Context->X8;
    EcContext->X0 = Context->X0;
    EcContext->X1 = Context->X1;
    EcContext->X27 = Context->X27;
    EcContext->Sp = Context->Sp;
    EcContext->Fp = Context->Fp;
    EcContext->X25 = Context->X25;
    EcContext->X26 = Context->X26;
    EcContext->X2 = Context->X2;
    EcContext->X3 = Context->X3;
    EcContext->X4 = Context->X4;
    EcContext->X5 = Context->X5;
    EcContext->X19 = Context->X19;
    EcContext->X20 = Context->X20;
    EcContext->X21 = Context->X21;
    EcContext->X22 = Context->X22;

    EcContext->Lr = Context->Lr;
    EcContext->X16_0 = LOWORD(Context->X16);
    EcContext->X6 = Context->X6;
    EcContext->X16_1 = HIWORD(Context->X16);
    EcContext->X7 = Context->X7;
    EcContext->X16_2 = LOWORD(Context->X16 >> 32);
    EcContext->X9 = Context->X9;
    EcContext->X16_3 = HIWORD(Context->X16 >> 32);
    EcContext->X10 = Context->X10;
    EcContext->X17_0 = LOWORD(Context->X17);
    EcContext->X11 = Context->X11;
    EcContext->X17_1 = HIWORD(Context->X17);
    EcContext->X12 = Context->X12;
    EcContext->X17_2 = LOWORD(Context->X17 >> 32);
    EcContext->X15 = Context->X15;
    EcContext->X17_3 = HIWORD(Context->X17 >> 32);

    EcContext->V[0] = Context->V[0];
    EcContext->V[1] = Context->V[1];
    EcContext->V[2] = Context->V[2];
    EcContext->V[3] = Context->V[3];
    EcContext->V[4] = Context->V[4];
    EcContext->V[5] = Context->V[5];
    EcContext->V[6] = Context->V[6];
    EcContext->V[7] = Context->V[7];
    EcContext->V[8] = Context->V[8];
    EcContext->V[9] = Context->V[9];
    EcContext->V[10] = Context->V[10];
    EcContext->V[11] = Context->V[11];
    EcContext->V[12] = Context->V[12];
    EcContext->V[13] = Context->V[13];
    EcContext->V[14] = Context->V[14];
    EcContext->V[15] = Context->V[15];
}

// rev from ntdll!RtlIsEcCode (jxy-s)
NTSTATUS PhIsEcCode(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID CodePointer,
    _Out_ PBOOLEAN IsEcCode
    )
{
    NTSTATUS status;
    PVOID pebBaseAddress;
    PVOID ecCodeBitMap;
    ULONG64 bitmap;

    *IsEcCode = FALSE;

    // hack (jxy-s)
    // 0x00007ffffffeffff = LdrpEcBitmapData.HighestAddress (MmHighestUserAddress)
    // 0x0000000000010000 = MM_LOWEST_USER_ADDRESS
    if ((ULONG_PTR)CodePointer > 0x00007ffffffeffff || (ULONG_PTR)CodePointer < 0x0000000000010000)
        return STATUS_INVALID_PARAMETER_2;

    if (!NT_SUCCESS(status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress)))
        return status;

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(pebBaseAddress, FIELD_OFFSET(PEB, EcCodeBitMap)),
        &ecCodeBitMap,
        sizeof(PVOID),
        NULL
        )))
        return status;

    if (!ecCodeBitMap)
        return STATUS_INVALID_PARAMETER_1;

    // each byte of bitmap indexes 8*4K = 2^15 byte span
    ecCodeBitMap = PTR_ADD_OFFSET(ecCodeBitMap, (ULONG_PTR)CodePointer >> 15);

    if (!NT_SUCCESS(status = NtReadVirtualMemory(
        ProcessHandle,
        ecCodeBitMap,
        &bitmap,
        sizeof(ULONG64),
        NULL
        )))
        return status;

    // index to the 4k page within the 8*4K span
    bitmap >>= (((ULONG_PTR)CodePointer >> PAGE_SHIFT) & 7);

    // test the specific page
    if (bitmap & 1)
        *IsEcCode = TRUE;

    return STATUS_SUCCESS;
}
#endif

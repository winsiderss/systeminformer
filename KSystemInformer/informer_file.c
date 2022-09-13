/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <dyndata.h>
#include <comms.h>

#include <trace.h>

PFLT_FILTER KphFltFilter = NULL;

PAGED_FILE();

static const UNICODE_STRING KphpDefaultInstaceName = RTL_CONSTANT_STRING(L"DefaultInstance");
static const UNICODE_STRING KphpAltitudeName = RTL_CONSTANT_STRING(L"Altitude");
static const UNICODE_STRING KphpFlagsName = RTL_CONSTANT_STRING(L"Flags");
static const DWORD KphpFlagsValue = 0;

/**
 * \brief Invoked when the mini-filter driver is asked to unload.
 *
 * \param[in] Flags Unused
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(PFLT_FILTER_UNLOAD_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS FLTAPI KphpFltFilterUnloadCallback(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Flags);

    KphTracePrint(TRACE_LEVEL_VERBOSE, INFORMER, "Filter unload invoked");

    KphCommsStop();

    return STATUS_SUCCESS;
}

/**
 * \brief Invoked when the mini-filter instance is asked to be removed.
 *
 * \param[in] FltObjects Unused
 * \param[in] Flags Unused
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS FLTAPI KphpFltInstanceQueryTeardownCallback(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  INFORMER,
                  "Filter query teardown invoked");

    return STATUS_SUCCESS;
}

static const FLT_REGISTRATION KphpFltRegistration =
{
    sizeof(FLT_REGISTRATION),
    FLT_REGISTRATION_VERSION,
    0,
    NULL,
    NULL,
    KphpFltFilterUnloadCallback,
    NULL,
    KphpFltInstanceQueryTeardownCallback,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/**
 * \brief Registers the driver with the mini-filter.
 *
 * \param[in] DriverObject Driver object of this driver.
 * \param[in] RegistryPath Registry path from the entry point.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphFltRegister(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE instancesKeyHandle;
    HANDLE defaultInstanceKeyHandle;
    WCHAR instancesBuffer[MAX_PATH];
    UNICODE_STRING keyName;
    BYTE objectInfoBuffer[MAX_PATH];
    POBJECT_NAME_INFORMATION objectInfo;
    ULONG returnLength;

    PAGED_PASSIVE();
    NT_ASSERT(!KphFltFilter);

    instancesKeyHandle = NULL;
    defaultInstanceKeyHandle = NULL;

    objectInfo = (POBJECT_NAME_INFORMATION)objectInfoBuffer;

    status = ObQueryNameString(DriverObject,
                               objectInfo,
                               sizeof(objectInfoBuffer),
                               &returnLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "ObQueryNameString failed: %!STATUS!",
                      status);
        goto Exit;
    }

    for (USHORT i = (objectInfo->Name.Length / sizeof(WCHAR)); i > 0; i--)
    {
        if (i == 1)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          INFORMER,
                          "Driver object name is invalid");

            status = STATUS_OBJECT_NAME_INVALID;
            goto Exit;
        }

        if (objectInfo->Name.Buffer[i - 1] == L'\\')
        {
            objectInfo->Name.Length -= (i * sizeof(WCHAR));
            RtlMoveMemory(objectInfo->Name.Buffer,
                          &objectInfo->Name.Buffer[i],
                          objectInfo->Name.Length);
            break;
        }
    }

    //
    // We null terminate this for the registry later.
    //
    NT_ASSERT(objectInfo->Name.MaximumLength >= (objectInfo->Name.Length + sizeof(WCHAR)));
    objectInfo->Name.Buffer[objectInfo->Name.Length / sizeof(WCHAR)] = L'\0';

    keyName.Buffer = instancesBuffer;
    keyName.Length = 0;
    keyName.MaximumLength = sizeof(instancesBuffer);

    status = RtlAppendUnicodeStringToString(&keyName, RegistryPath);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "RtlAppendUnicodeStringToString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlAppendUnicodeToString(&keyName, L"\\Instances");
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "RtlAppendUnicodeToString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &keyName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = ZwCreateKey(&instancesKeyHandle,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "ZwCreateKey failed: %!STATUS!",
                      status);

        instancesKeyHandle = NULL;
        goto Exit;
    }

    //
    // We guarantee this above.
    //
    NT_ASSERT(objectInfo->Name.MaximumLength >= (objectInfo->Name.Length + sizeof(WCHAR)));
    NT_ASSERT(objectInfo->Name.Buffer[objectInfo->Name.Length / sizeof(WCHAR)] == L'\0');
    status = ZwSetValueKey(instancesKeyHandle,
                           (PUNICODE_STRING)&KphpDefaultInstaceName,
                           0,
                           REG_SZ,
                           objectInfo->Name.Buffer,
                           objectInfo->Name.Length + sizeof(WCHAR));
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "ZwSetValueKey failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlAppendUnicodeToString(&keyName, L"\\");
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "RtlAppendUnicodeToString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlAppendUnicodeStringToString(&keyName, &objectInfo->Name);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "RtlAppendUnicodeStringToString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &keyName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = ZwCreateKey(&defaultInstanceKeyHandle,
                         KEY_ALL_ACCESS,
                         &objectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "ZwCreateKey failed: %!STATUS!",
                      status);

        defaultInstanceKeyHandle = NULL;
        goto Exit;
    }

    //
    // Our registry reading function for this parameter guarantees this.
    //
    NT_ASSERT(KphDynAltitude);
    NT_ASSERT(KphDynAltitude->MaximumLength >= (KphDynAltitude->Length + sizeof(WCHAR)));
    NT_ASSERT(KphDynAltitude->Buffer[KphDynAltitude->Length / sizeof(WCHAR)] == L'\0');
    status = ZwSetValueKey(defaultInstanceKeyHandle,
                           (PUNICODE_STRING)&KphpAltitudeName,
                           0,
                           REG_SZ,
                           KphDynAltitude->Buffer,
                           KphDynAltitude->Length + sizeof(WCHAR));
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "ZwSetValueKey failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = ZwSetValueKey(defaultInstanceKeyHandle,
                           (PUNICODE_STRING)&KphpFlagsName,
                           0,
                           REG_DWORD,
                           (PVOID)&KphpFlagsValue,
                           sizeof(KphpFlagsValue));
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "ZwSetValueKey failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = FltRegisterFilter(DriverObject,
                               &KphpFltRegistration,
                               &KphFltFilter);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "FltRegisterFilter failed: %!STATUS!",
                      status);

        KphFltFilter = NULL;
        goto Exit;
    }

    status = KphCommsStart();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "KphCommsStart failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    if (instancesKeyHandle)
    {
        ObCloseHandle(instancesKeyHandle, KernelMode);
    }

    if (defaultInstanceKeyHandle)
    {
        ObCloseHandle(defaultInstanceKeyHandle, KernelMode);
    }

    return status;
}

/**
 * \brief Unregisters this driver with the mini-filter.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphFltUnregister(
    VOID
    )
{
    PAGED_PASSIVE();

    if (!KphFltFilter)
    {
        return;
    }

    FltUnregisterFilter(KphFltFilter);
    KphFltFilter = NULL;
}

/**
 * \brief Begins informing on file activity from the mini-filter.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphFltInformerStart(
    VOID
    )
{
    PAGED_PASSIVE();

    NT_ASSERT(KphFltFilter);

    return FltStartFiltering(KphFltFilter);
}

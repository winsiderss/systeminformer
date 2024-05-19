/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2023
 *
 */

#include <kph.h>
#include <comms.h>
#include <informer_filep.h>

#include <trace.h>

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpDefaultInstaceName = RTL_CONSTANT_STRING(L"DefaultInstance");
static const UNICODE_STRING KphpAltitudeName = RTL_CONSTANT_STRING(L"Altitude");
static const UNICODE_STRING KphpFlagsName = RTL_CONSTANT_STRING(L"Flags");
static const DWORD KphpFlagsValue = 0;
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
PFLT_FILTER KphFltFilter = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

PAGED_FILE();

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
    PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Flags);

    KphTracePrint(TRACE_LEVEL_VERBOSE, INFORMER, "Filter unload invoked");

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
    PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  INFORMER,
                  "Filter query teardown invoked");

    if (KphIsDriverUnloadProtected())
    {
        return STATUS_FLT_DO_NOT_DETACH;
    }

    return STATUS_SUCCESS;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const FLT_OPERATION_REGISTRATION KphpFltOperationRegistration[] =
{
{ IRP_MJ_CREATE,                              0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_CREATE_NAMED_PIPE,                   0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_CLOSE,                               0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_READ,                                0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_WRITE,                               0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_QUERY_INFORMATION,                   0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_SET_INFORMATION,                     0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_QUERY_EA,                            0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_SET_EA,                              0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_FLUSH_BUFFERS,                       0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_QUERY_VOLUME_INFORMATION,            0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_SET_VOLUME_INFORMATION,              0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_DIRECTORY_CONTROL,                   0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_FILE_SYSTEM_CONTROL,                 0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_DEVICE_CONTROL,                      0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_INTERNAL_DEVICE_CONTROL,             0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_SHUTDOWN,                            0, KphpFltPreOp, NULL,          0 },
{ IRP_MJ_LOCK_CONTROL,                        0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_CLEANUP,                             0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_CREATE_MAILSLOT,                     0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_QUERY_SECURITY,                      0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_SET_SECURITY,                        0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_POWER,                               0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_SYSTEM_CONTROL,                      0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_DEVICE_CHANGE,                       0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_QUERY_QUOTA,                         0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_SET_QUOTA,                           0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_PNP,                                 0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION, 0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION, 0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_ACQUIRE_FOR_MOD_WRITE,               0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_RELEASE_FOR_MOD_WRITE,               0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_ACQUIRE_FOR_CC_FLUSH,                0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_RELEASE_FOR_CC_FLUSH,                0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_QUERY_OPEN,                          0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,           0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_NETWORK_QUERY_OPEN,                  0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_MDL_READ,                            0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_MDL_READ_COMPLETE,                   0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_PREPARE_MDL_WRITE,                   0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_MDL_WRITE_COMPLETE,                  0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_VOLUME_MOUNT,                        0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_VOLUME_DISMOUNT,                     0, KphpFltPreOp, KphpFltPostOp, 0 },
{ IRP_MJ_OPERATION_END }
};
static const FLT_CONTEXT_REGISTRATION KphpFltContextRegistration[] =
{
    {
        FLT_STREAMHANDLE_CONTEXT,
        0,
        NULL,
        FLT_VARIABLE_SIZED_CONTEXTS,
        KPH_TAG_FLT_STREAMHANDLE_CONTEXT
    },
    { FLT_CONTEXT_END }
};
static const FLT_REGISTRATION KphpFltRegistration =
{
    sizeof(FLT_REGISTRATION),
    FLT_REGISTRATION_VERSION,
    (FLTFL_REGISTRATION_SUPPORT_NPFS_MSFS |
     FLTFL_REGISTRATION_SUPPORT_DAX_VOLUME |
     FLTFL_REGISTRATION_SUPPORT_WCOS),
    KphpFltContextRegistration,
    KphpFltOperationRegistration,
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
KPH_PROTECTED_DATA_SECTION_RO_POP();

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

    PAGED_CODE_PASSIVE();
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "ObQueryNameString failed: %!STATUS!",
                      status);
        goto Exit;
    }

    for (USHORT i = (objectInfo->Name.Length / sizeof(WCHAR)); i > 0; i--)
    {
        if (i == 1)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "RtlAppendUnicodeStringToString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlAppendUnicodeToString(&keyName, L"\\Instances");
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "ZwSetValueKey failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlAppendUnicodeToString(&keyName, L"\\");
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "RtlAppendUnicodeToString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlAppendUnicodeStringToString(&keyName, &objectInfo->Name);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "ZwCreateKey failed: %!STATUS!",
                      status);

        defaultInstanceKeyHandle = NULL;
        goto Exit;
    }

    NT_ASSERT(KphAltitude);
    NT_ASSERT(KphAltitude->MaximumLength >= (KphAltitude->Length + sizeof(WCHAR)));
    NT_ASSERT(KphAltitude->Buffer[KphAltitude->Length / sizeof(WCHAR)] == L'\0');
    status = ZwSetValueKey(defaultInstanceKeyHandle,
                           (PUNICODE_STRING)&KphpAltitudeName,
                           0,
                           REG_SZ,
                           KphAltitude->Buffer,
                           KphAltitude->Length + sizeof(WCHAR));
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "FltRegisterFilter failed: %!STATUS!",
                      status);

        KphFltFilter = NULL;
        goto Exit;
    }

    status = KphCommsStart();
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
    PAGED_CODE_PASSIVE();

    KphCommsStop();

    if (!KphFltFilter)
    {
        return;
    }

    FltUnregisterFilter(KphFltFilter);

    KphpFltCleanupFileNameCache();
    KphpFltCleanupFileOp();
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
    PAGED_CODE_PASSIVE();

    NT_ASSERT(KphFltFilter);

    KphpFltInitializeFileOp();
    KphpFltInitializeFileNameCache();

    return FltStartFiltering(KphFltFilter);
}

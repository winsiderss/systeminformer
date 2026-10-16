/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <kph.h>
#include <kphhw.h>
#include <kphdyndata.h>
#include <smbios.h>

#include <trace.h>

#define KPH_HW_FNV1A64_OFFSET_BASIS 0xcbf29ce484222325ull
#define KPH_HW_FNV1A64_PRIME        0x00000100000001b3ull
#define KPH_HW_SMBIOS_PROVIDER      'RSMB'
#define KPH_HW_ENUMERATE_STACK      16
#define KPH_HW_ENUMERATE_SLACK      8

KPH_PROTECTED_DATA_SECTION_PUSH();
static KPH_HW_CPU_IDENTITY KphpHwCpuIdentity = { 0 };
static KPH_DYN_HW_BOARD_IDENTITY KphpHwBoardIdentity = { 0 };
static UCHAR KphpHwChassisType = 0;
KPH_PROTECTED_DATA_SECTION_POP();

KPH_PAGED_FILE();

/**
 * \brief Retrieves the cached CPU identity.
 *
 * \return The CPU identity, zeroed on ARM64.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PCKPH_HW_CPU_IDENTITY KphGetHardwareCpuIdentity(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return &KphpHwCpuIdentity;
}

/**
 * \brief Retrieves the cached board identity.
 *
 * \return The baseboard and system hashes, each zero when absent.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PCKPH_DYN_HW_BOARD_IDENTITY KphGetHardwareBoardIdentity(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return &KphpHwBoardIdentity;
}

/**
 * \brief Retrieves the cached SMBIOS chassis type.
 *
 * \return SMBIOS type 3 offset 05h bits 6:0, zero when absent.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
UCHAR KphGetHardwareChassisType(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpHwChassisType;
}

/**
 * \brief Measures the string set of an SMBIOS structure.
 *
 * \param[in] Strings The string set; a NUL follows the table in its buffer.
 * \param[in] Remaining The number of bytes from the set to the table end.
 * \param[out] Length Receives the length of the set, through its double NUL.
 *
 * \return TRUE if the set ends within the table, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphpHwMeasureSmbiosStrings(
    _In_ PCSTR Strings,
    _In_ ULONG Remaining,
    _Out_ PULONG Length
    )
{
    ANSI_STRING string;
    ULONG offset;

    KPH_PAGED_CODE_PASSIVE();

    offset = 0;

    do
    {
        if (offset >= Remaining)
        {
            return FALSE;
        }

        RtlInitAnsiString(&string, &Strings[offset]);

        if (string.Length >= (Remaining - offset))
        {
            return FALSE;
        }

        offset += (string.Length + (ULONG)sizeof(ANSI_NULL));
    } while ((string.Length > 0) || (offset == sizeof(ANSI_NULL)));

    *Length = offset;

    return TRUE;
}

/**
 * \brief Mixes a trimmed, ASCII upper-cased SMBIOS string into a board hash.
 *
 * \param[in,out] Hash The FNV-1a-64 hash to mix into.
 * \param[in] Strings The string set of the structure.
 * \param[in] Length The length of the string set, which ends in a double NUL.
 * \param[in] Index The one-based string number; zero is the empty string.
 *
 * \return TRUE if a non-empty string was mixed in, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwHashSmbiosString(
    _Inout_ PULONG64 Hash,
    _In_reads_bytes_(Length) PCSTR Strings,
    _In_ ULONG Length,
    _In_ UCHAR Index
    )
{
    ANSI_STRING string;
    ULONG offset;
    USHORT start;
    USHORT end;

    KPH_PAGED_CODE_PASSIVE();

    if (Index == SMBIOS_INVALID_STRING)
    {
        return FALSE;
    }

    offset = 0;

    RtlInitAnsiString(&string, Strings);

    while (string.Length > 0)
    {
        if (--Index == 0)
        {
            start = 0;
            end = string.Length;

            while ((start < end) &&
                   ((string.Buffer[start] == ' ') ||
                    (string.Buffer[start] == '\t')))
            {
                start++;
            }

            while ((end > start) &&
                   ((string.Buffer[end - 1] == ' ') ||
                    (string.Buffer[end - 1] == '\t')))
            {
                end--;
            }

            for (USHORT i = start; i < end; i++)
            {
                UCHAR c;

                c = (UCHAR)string.Buffer[i];

                if ((c >= 'a') && (c <= 'z'))
                {
                    c -= ('a' - 'A');
                }

                *Hash = (*Hash ^ c) * KPH_HW_FNV1A64_PRIME;
            }

            return (end > start);
        }

        offset += (string.Length + (ULONG)sizeof(ANSI_NULL));

        if (offset >= Length)
        {
            break;
        }

        RtlInitAnsiString(&string, &Strings[offset]);
    }

    return FALSE;
}

/**
 * \brief Hashes the Manufacturer and Product strings of an SMBIOS structure.
 *
 * \param[in] Strings The string set of the structure.
 * \param[in] Length The length of the string set, which ends in a double NUL.
 * \param[in] Manufacturer The string number of the manufacturer.
 * \param[in] Product The string number of the product.
 *
 * \return The board hash, zero when both strings are empty.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG64 KphpHwHashBoard(
    _In_reads_bytes_(Length) PCSTR Strings,
    _In_ ULONG Length,
    _In_ UCHAR Manufacturer,
    _In_ UCHAR Product
    )
{
    ULONG64 hash;
    BOOLEAN manufacturerMixed;
    BOOLEAN productMixed;

    KPH_PAGED_CODE_PASSIVE();

    hash = KPH_HW_FNV1A64_OFFSET_BASIS;

    manufacturerMixed = KphpHwHashSmbiosString(&hash,
                                               Strings,
                                               Length,
                                               Manufacturer);
    hash *= KPH_HW_FNV1A64_PRIME;
    productMixed = KphpHwHashSmbiosString(&hash, Strings, Length, Product);

    return ((manufacturerMixed | productMixed) ? hash : 0);
}

/**
 * \brief Caches the board identity from SMBIOS types 1, 2 and 3.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwInitializeBoardIdentity(
    VOID
    )
{
    NTSTATUS status;
    SYSTEM_FIRMWARE_TABLE_INFORMATION query;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    ULONG infoLength;
    ULONG allocationLength;
    ULONG tableLength;
    PRAW_SMBIOS_DATA smbios;
    PUCHAR data;
    ULONG dataOffset;
    ULONG dataLength;
    ULONG offset;
    BOOLEAN baseboardFound;
    BOOLEAN systemFound;
    BOOLEAN chassisFound;

    KPH_PAGED_CODE_PASSIVE();

    info = NULL;
    baseboardFound = FALSE;
    systemFound = FALSE;
    chassisFound = FALSE;

    RtlZeroMemory(&query, sizeof(query));
    query.ProviderSignature = KPH_HW_SMBIOS_PROVIDER;
    query.Action = SystemFirmwareTable_Get;
    query.TableID = 0;
    query.TableBufferLength = 0;

    status = ZwQuerySystemInformation(SystemFirmwareTableInformation,
                                      &query,
                                      sizeof(query),
                                      NULL);
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ZwQuerySystemInformation failed: %!STATUS!",
                      status);

        goto Exit;
    }

    tableLength = query.TableBufferLength;

    status = RtlULongAdd(FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION,
                                      TableBuffer),
                         tableLength,
                         &infoLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlULongAdd(infoLength, sizeof(ANSI_NULL), &allocationLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    info = KphAllocatePaged(allocationLength, KPH_TAG_HW_SMBIOS);
    if (!info)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate SMBIOS table");

        goto Exit;
    }

    info->ProviderSignature = KPH_HW_SMBIOS_PROVIDER;
    info->Action = SystemFirmwareTable_Get;
    info->TableID = 0;
    info->TableBufferLength = tableLength;

    status = ZwQuerySystemInformation(SystemFirmwareTableInformation,
                                      info,
                                      infoLength,
                                      NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ZwQuerySystemInformation failed: %!STATUS!",
                      status);

        goto Exit;
    }

    dataOffset = FIELD_OFFSET(RAW_SMBIOS_DATA, SMBIOSTableData);

    if ((info->TableBufferLength > tableLength) ||
        (info->TableBufferLength < dataOffset))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMBIOS table length %lu invalid, buffer %lu",
                      info->TableBufferLength,
                      tableLength);

        goto Exit;
    }

    smbios = (PRAW_SMBIOS_DATA)info->TableBuffer;
    data = smbios->SMBIOSTableData;
    dataLength = info->TableBufferLength - dataOffset;

    if (smbios->Length < dataLength)
    {
        dataLength = smbios->Length;
    }

    offset = 0;

    while ((dataLength - offset) >= sizeof(SMBIOS_HEADER))
    {
        PSMBIOS_HEADER header;
        PCSTR strings;
        ULONG stringsOffset;
        ULONG stringsLength;

        header = (PSMBIOS_HEADER)&data[offset];

        if ((header->Length < sizeof(SMBIOS_HEADER)) ||
            (header->Length > (dataLength - offset)))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBIOS structure at %lu has invalid length %lu",
                          offset,
                          (ULONG)header->Length);

            break;
        }

        stringsOffset = (offset + header->Length);
        strings = (PCSTR)&data[stringsOffset];

        if (!KphpHwMeasureSmbiosStrings(strings,
                                        (dataLength - stringsOffset),
                                        &stringsLength))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBIOS structure at %lu has unterminated strings",
                          offset);

            break;
        }

        if (header->Type == SMBIOS_END_OF_TABLE_TYPE)
        {
            break;
        }

        if ((header->Type == SMBIOS_BASEBOARD_INFORMATION_TYPE) &&
            !baseboardFound)
        {
            PSMBIOS_BASEBOARD_INFORMATION baseboard;

            baseboard = (PSMBIOS_BASEBOARD_INFORMATION)header;
            baseboardFound = TRUE;

            if (RTL_CONTAINS_FIELD(baseboard, header->Length, Product))
            {
                KphpHwBoardIdentity.BaseboardHash = KphpHwHashBoard(
                                                        strings,
                                                        stringsLength,
                                                        baseboard->Manufacturer,
                                                        baseboard->Product);
            }
        }
        else if ((header->Type == SMBIOS_CHASSIS_INFORMATION_TYPE) &&
                 !chassisFound)
        {
            PSMBIOS_CHASSIS_INFORMATION chassis;

            chassis = (PSMBIOS_CHASSIS_INFORMATION)header;
            chassisFound = TRUE;

            if (RTL_CONTAINS_FIELD(chassis, header->Length, Chassis))
            {
                KphpHwChassisType = chassis->Chassis.Type;
            }
        }
        else if ((header->Type == SMBIOS_SYSTEM_INFORMATION_TYPE) &&
                 !systemFound)
        {
            PSMBIOS_SYSTEM_INFORMATION system;

            system = (PSMBIOS_SYSTEM_INFORMATION)header;
            systemFound = TRUE;

            if (RTL_CONTAINS_FIELD(system, header->Length, ProductName))
            {
                KphpHwBoardIdentity.SystemHash = KphpHwHashBoard(
                                                           strings,
                                                           stringsLength,
                                                           system->Manufacturer,
                                                           system->ProductName);
            }
        }

        offset = (stringsOffset + stringsLength);
    }

Exit:

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "Board baseboard hash 0x%I64x system hash 0x%I64x chassis "
                  "type 0x%02lx",
                  KphpHwBoardIdentity.BaseboardHash,
                  KphpHwBoardIdentity.SystemHash,
                  (ULONG)KphpHwChassisType);

    if (info)
    {
        KphFree(info, KPH_TAG_HW_SMBIOS);
    }
}

/**
 * \brief References a driver object by name.
 *
 * \param[in] Name The driver object name, e.g. \Driver\ACPI.
 * \param[out] Driver Receives the referenced driver object, NULL on failure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReferenceDriver(
    _In_ PCUNICODE_STRING Name,
    _Outptr_ PDRIVER_OBJECT* Driver
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    status = ObReferenceObjectByName((PUNICODE_STRING)Name,
                                     OBJ_DONT_REPARSE,
                                     NULL,
                                     0,
                                     *IoDriverObjectType,
                                     KernelMode,
                                     NULL,
                                     (PVOID*)Driver);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ObReferenceObjectByName failed: %!STATUS!",
                      status);

        *Driver = NULL;
        return STATUS_NOT_FOUND;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Lists and references the device objects of a driver.
 *
 * \param[in] Driver The driver object.
 * \param[out] List Receives the referenced device objects.
 * \param[in] Length The length of the list buffer, in bytes.
 * \param[out] Count Receives the number of device objects; when the buffer is
 * too small, the number needed, with none of them left referenced.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwListDriverDevices(
    _In_ PDRIVER_OBJECT Driver,
    _Out_writes_bytes_to_(Length, *Count * sizeof(PDEVICE_OBJECT))
        PDEVICE_OBJECT* List,
    _In_ ULONG Length,
    _Out_ PULONG Count
    )
{
    NTSTATUS status;
    ULONG capacity;

    KPH_PAGED_CODE_PASSIVE();

    status = IoEnumerateDeviceObjectList(Driver, List, Length, Count);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        capacity = (ULONG)(Length / sizeof(PDEVICE_OBJECT));

        for (ULONG i = 0; (i < *Count) && (i < capacity); i++)
        {
            ObDereferenceObject(List[i]);
        }
    }

    return status;
}

/**
 * \brief Enumerates the device objects of a driver.
 *
 * \details Each device object is referenced only for the duration of its
 * callback; a callback that keeps one references it.
 *
 * \param[in] Driver The driver object.
 * \param[in] Callback Invoked for each device object; returns TRUE to stop.
 * \param[in] Context Optional context passed to the callback.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwEnumerateDriverDevices(
    _In_ PDRIVER_OBJECT Driver,
    _In_ PKPH_HW_ENUMERATE_DEVICES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    BYTE stackBuffer[KPH_HW_ENUMERATE_STACK * sizeof(PDEVICE_OBJECT)];
    PDEVICE_OBJECT* list;
    ULONG length;
    ULONG count;
    BOOLEAN stopped;

    KPH_PAGED_CODE_PASSIVE();

    list = KphAllocateNPagedA(sizeof(stackBuffer),
                              KPH_TAG_HW_DOLIST,
                              stackBuffer);
    if (!list)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate device object list");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpHwListDriverDevices(Driver,
                                     list,
                                     sizeof(stackBuffer),
                                     &count);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        KphFreeA(list, KPH_TAG_HW_DOLIST, stackBuffer);
        list = NULL;

        status = RtlULongAdd(count, KPH_HW_ENUMERATE_SLACK, &count);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "RtlULongAdd failed: %!STATUS!",
                          status);

            goto Exit;
        }

        status = RtlULongMult(count, sizeof(PDEVICE_OBJECT), &length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "RtlULongMult failed: %!STATUS!",
                          status);

            goto Exit;
        }

        list = KphAllocateNPagedA(length, KPH_TAG_HW_DOLIST, stackBuffer);
        if (!list)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to allocate device object list");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        status = KphpHwListDriverDevices(Driver, list, length, &count);
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Device object list grew to %lu entries",
                          count);

            status = STATUS_RETRY;
            goto Exit;
        }
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoEnumerateDeviceObjectList failed: %!STATUS!",
                      status);

        goto Exit;
    }

    stopped = FALSE;

    for (ULONG i = 0; i < count; i++)
    {
        PDEVICE_OBJECT deviceObject;

#pragma prefast(suppress: 6385) // count entries fit when the list succeeds
        deviceObject = list[i];

        if (!stopped && Callback(deviceObject, Context))
        {
            stopped = TRUE;
        }

        ObDereferenceObject(deviceObject);
    }

    status = STATUS_SUCCESS;

Exit:

    if (list)
    {
        KphFreeA(list, KPH_TAG_HW_DOLIST, stackBuffer);
    }

    return status;
}

/**
 * \brief Checks whether the PnP manager expects a device never to be removed.
 *
 * \param[in] Pdo The physical device object.
 *
 * \return TRUE if the removal policy is RemovalPolicyExpectNoRemoval.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwExpectsNoRemoval(
    _In_ PDEVICE_OBJECT Pdo
    )
{
    NTSTATUS status;
    DEVICE_REMOVAL_POLICY policy;
    ULONG length;

    KPH_PAGED_CODE_PASSIVE();

    status = IoGetDeviceProperty(Pdo,
                                 DevicePropertyRemovalPolicy,
                                 sizeof(policy),
                                 &policy,
                                 &length);

    return (NT_SUCCESS(status) && (policy == RemovalPolicyExpectNoRemoval));
}

/**
 * \brief Retrieves a boot configuration resource list of a device.
 *
 * \param[in] Pdo The physical device object.
 * \param[in] Property DevicePropertyBootConfiguration or
 * DevicePropertyBootConfigurationTranslated.
 * \param[out] List Receives the list, freed with KPH_TAG_HW_BOOT_CONFIG,
 * untouched on failure.
 * \param[out] Length Receives the length of the list, untouched on failure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwQueryBootConfiguration(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ DEVICE_REGISTRY_PROPERTY Property,
    _Outptr_result_bytebuffer_(*Length) PCM_RESOURCE_LIST* List,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    PCM_RESOURCE_LIST list;
    ULONG listLength;

    KPH_PAGED_CODE_PASSIVE();

    list = NULL;
    listLength = 0;

    status = IoGetDeviceProperty(Pdo, Property, 0, NULL, &listLength);
    if ((status != STATUS_BUFFER_TOO_SMALL) || (listLength == 0))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty returned: %!STATUS!",
                      status);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    list = KphAllocatePaged(listLength, KPH_TAG_HW_BOOT_CONFIG);
    if (!list)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate boot configuration");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = IoGetDeviceProperty(Pdo, Property, listLength, list, &listLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty failed: %!STATUS!",
                      status);

        goto Exit;
    }

    *List = list;
    list = NULL;
    *Length = listLength;

Exit:

    if (list)
    {
        KphFree(list, KPH_TAG_HW_BOOT_CONFIG);
    }

    return status;
}

/**
 * \brief Finds a memory descriptor of a resource list.
 *
 * \param[in] List The resource list.
 * \param[in] ListLength The length of the list.
 * \param[in] Mode The search mode.
 * \param[in,out] Ordinal The ordinal to find, or receives the ordinal found.
 * \param[in,out] Start The start to find, or receives the start found.
 * \param[out] Length Receives the length of the descriptor found.
 * \param[out] Flags Optionally receives the CM_RESOURCE_MEMORY_* flags.
 *
 * \return TRUE if a memory descriptor was found.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphHwFindMemoryDescriptor(
    _In_reads_bytes_(ListLength) PCM_RESOURCE_LIST List,
    _In_ ULONG ListLength,
    _In_ KPH_HW_FIND_MEMORY Mode,
    _Inout_ PULONG Ordinal,
    _Inout_ PULONG64 Start,
    _Out_ PULONG64 Length,
    _Out_opt_ PUSHORT Flags
    )
{
    ULONG fullHeaderLength;
    ULONG offset;
    ULONG ordinal;
    ULONG memoryOrdinal;

    KPH_PAGED_CODE_PASSIVE();

    if (ListLength < (ULONG)FIELD_OFFSET(CM_RESOURCE_LIST, List))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Resource list of %lu bytes is truncated",
                      ListLength);

        return FALSE;
    }

    fullHeaderLength = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                    PartialResourceList.PartialDescriptors);
    offset = FIELD_OFFSET(CM_RESOURCE_LIST, List);
    ordinal = 0;
    memoryOrdinal = 0;

    for (ULONG i = 0; i < List->Count; i++)
    {
        PCM_FULL_RESOURCE_DESCRIPTOR full;
        ULONG partialCount;

        if ((ListLength - offset) < fullHeaderLength)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Resource list truncated at %lu",
                          offset);

            return FALSE;
        }

        full = Add2Ptr(List, offset);
        partialCount = full->PartialResourceList.Count;
        offset += fullHeaderLength;

        for (ULONG j = 0; j < partialCount; j++, ordinal++)
        {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
            ULONG64 start;
            ULONG64 length;

            if ((ListLength - offset) < sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "Resource list truncated at %lu",
                              offset);

                return FALSE;
            }

            partial = Add2Ptr(List, offset);
            offset += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

            if (partial->Type == CmResourceTypeDeviceSpecific)
            {
                ULONG dataSize;

                dataSize = partial->u.DeviceSpecificData.DataSize;

                if ((ListLength - offset) < dataSize)
                {
                    KphTracePrint(TRACE_LEVEL_VERBOSE,
                                  HARDWARE,
                                  "Resource list device data of %lu bytes "
                                  "overruns at %lu",
                                  dataSize,
                                  offset);

                    return FALSE;
                }

                offset += dataSize;
            }

            if ((Mode == KPH_HW_FIND_MEMORY_BY_ORDINAL) &&
                (ordinal != *Ordinal))
            {
                continue;
            }

            if ((partial->Type != CmResourceTypeMemory) &&
                (partial->Type != CmResourceTypeMemoryLarge))
            {
                if (Mode == KPH_HW_FIND_MEMORY_BY_ORDINAL)
                {
                    return FALSE;
                }

                continue;
            }

            start = 0;
            length = RtlCmDecodeMemIoResource(partial, &start);

            if (Mode == KPH_HW_FIND_MEMORY_BY_ORDINAL)
            {
                *Start = start;
                *Length = length;
                if (Flags)
                {
                    *Flags = partial->Flags;
                }

                return TRUE;
            }

            if (Mode == KPH_HW_FIND_MEMORY_BY_MEMORY_ORDINAL)
            {
                if (memoryOrdinal++ != *Ordinal)
                {
                    continue;
                }

                *Ordinal = ordinal;
                *Start = start;
                *Length = length;
                if (Flags)
                {
                    *Flags = partial->Flags;
                }

                return TRUE;
            }

            if (start == *Start)
            {
                *Ordinal = ordinal;
                *Length = length;
                if (Flags)
                {
                    *Flags = partial->Flags;
                }

                return TRUE;
            }
        }
    }

    return FALSE;
}

#if defined(_M_X64)
/**
 * \brief Evaluates a CPUID gate on the current processor.
 *
 * \param[in] Cpu The cached CPU identity.
 * \param[in] Gate The gate, KPH_DYN_HW_GATE_CPUID or KPH_DYN_HW_GATE_NONE.
 *
 * \return TRUE if the gate holds on the current processor, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwCpuidGatePasses(
    _In_ PCKPH_HW_CPU_IDENTITY Cpu,
    _In_ PCKPH_DYN_HW_GATE Gate
    )
{
    KPH_CPUID cpuid;
    ULONG field;

    KPH_PAGED_CODE_PASSIVE();

    if (Gate->Kind != KPH_DYN_HW_GATE_CPUID)
    {
        NT_ASSERT(Gate->Kind == KPH_DYN_HW_GATE_NONE);
        return TRUE;
    }

    if (Gate->Leaf >= 0x80000000)
    {
        if (Gate->Leaf > Cpu->MaxExtendedLeaf)
        {
            return FALSE;
        }
    }
    else if (Gate->Leaf > Cpu->MaxBasicLeaf)
    {
        return FALSE;
    }

    NT_ASSERT(Gate->Register <= KPH_DYN_HW_CPUID_EDX);
    NT_ASSERT((Gate->Width >= 1) && (Gate->Width <= 32));
    NT_ASSERT(Gate->Shift <= (32 - Gate->Width));

    KphCpuId(&cpuid, (INT32)Gate->Leaf, (INT32)Gate->SubLeaf);

    field = (ULONG)cpuid.Data[Gate->Register] >> Gate->Shift;
    if (Gate->Width < 32)
    {
        field &= ((1ul << Gate->Width) - 1);
    }

    return ((field >= Gate->Value) && (field <= Gate->ValueMax));
}
#endif

/**
 * \brief Checks whether a processor number names an active processor.
 *
 * \param[in] Processor The processor number to check.
 *
 * \return TRUE if the processor is active, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwIsActiveProcessor(
    _In_ const PROCESSOR_NUMBER* Processor
    )
{
    KAFFINITY active;

    KPH_PAGED_CODE_PASSIVE();

    if (Processor->Group >= KeQueryActiveGroupCount())
    {
        return FALSE;
    }

    if (Processor->Number >= MAXIMUM_PROC_PER_GROUP)
    {
        return FALSE;
    }

    active = KeQueryGroupAffinity(Processor->Group);

    return BooleanFlagOn(active, (KAFFINITY)1 << Processor->Number);
}

/**
 * \brief Pins the current thread to a processor.
 *
 * \param[in] Processor The active processor to pin to.
 * \param[out] Previous Receives the affinity to pass to KphHwUnpinProcessor.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwPinProcessor(
    _In_ const PROCESSOR_NUMBER* Processor,
    _Out_ PGROUP_AFFINITY Previous
    )
{
    GROUP_AFFINITY affinity;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&affinity, sizeof(affinity));
    affinity.Group = Processor->Group;
    affinity.Mask = (KAFFINITY)1 << Processor->Number;

    KeSetSystemGroupAffinityThread(&affinity, Previous);

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "Pinned to processor %lu:%lu",
                  (ULONG)Processor->Group,
                  (ULONG)Processor->Number);
}

/**
 * \brief Reverts a pin made by KphHwPinProcessor.
 *
 * \param[in] Previous The affinity from KphHwPinProcessor.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwUnpinProcessor(
    _In_ PGROUP_AFFINITY Previous
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KeRevertToUserGroupAffinityThread(Previous);

    KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Unpinned from processor");
}

/**
 * \brief Captures an optional timeout as an interrupt time deadline.
 *
 * \param[in] Timeout Optional timeout as KeWaitForSingleObject takes it; NULL
 * is infinite, negative is relative, positive is absolute, zero polls once.
 * \param[out] Deadline Receives the deadline, MAXULONG64 when infinite.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwCaptureDeadline(
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_ PULONG64 Deadline,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    LARGE_INTEGER timeout;
    LARGE_INTEGER systemTime;
    ULONG64 interval;

    KPH_PAGED_CODE_PASSIVE();

    *Deadline = MAXULONG64;

    if (!Timeout)
    {
        return STATUS_SUCCESS;
    }

    status = KphReadLargeIntegerFromMode(&timeout, Timeout, AccessMode);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (timeout.QuadPart < 0)
    {
        interval = (0ull - (ULONG64)timeout.QuadPart);
    }
    else
    {
        KeQuerySystemTime(&systemTime);

        if (timeout.QuadPart > systemTime.QuadPart)
        {
            interval = (ULONG64)(timeout.QuadPart - systemTime.QuadPart);
        }
        else
        {
            interval = 0;
        }
    }

    status = RtlULongLongAdd(KeQueryInterruptTime(), interval, Deadline);
    if (!NT_SUCCESS(status))
    {
        *Deadline = MAXULONG64;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Checks whether a deadline from KphHwCaptureDeadline has passed.
 *
 * \param[in] Deadline The deadline.
 *
 * \return TRUE if the deadline has passed, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphHwDeadlinePassed(
    _In_ ULONG64 Deadline
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return (KeQueryInterruptTime() >= Deadline);
}

/**
 * \brief Initializes the hardware infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardware(
    VOID
    )
{
#if defined(_M_X64)
    KPH_CPUID cpuid0;
    KPH_CPUID_1 cpuid1;
    KPH_CPUID cpuidExt;
    PKPH_HW_CPU_IDENTITY identity;
    CHAR vendor[sizeof(identity->Vendor) + sizeof(ANSI_NULL)];
#endif

    KPH_PAGED_CODE_PASSIVE();

#if defined(_M_X64)
    identity = &KphpHwCpuIdentity;

    KphCpuId(&cpuid0, 0, 0);
    KphCpuId(&cpuid1, 1, 0);
    KphCpuId(&cpuidExt, 0x80000000, 0);

    identity->Vendor[0] = cpuid0.EBX;
    identity->Vendor[1] = cpuid0.EDX;
    identity->Vendor[2] = cpuid0.ECX;

    identity->DisplayFamily = (USHORT)cpuid1.Family;
    if (cpuid1.Family == 0xF)
    {
        identity->DisplayFamily += (USHORT)cpuid1.ExtendedFamily;
    }

    identity->DisplayModel = (UCHAR)cpuid1.Model;
    if ((cpuid1.Family == 0x6) || (cpuid1.Family == 0xF))
    {
        identity->DisplayModel |= (UCHAR)(cpuid1.ExtendedModel << 4);
    }

    identity->Stepping = (UCHAR)cpuid1.Stepping;
    identity->MaxBasicLeaf = cpuid0.EAX;
    identity->MaxExtendedLeaf = cpuidExt.EAX;

    identity->PhysicalAddressBits = 36;

    if (identity->MaxExtendedLeaf >= 0x80000008)
    {
        KPH_CPUID cpuidAddress;

        KphCpuId(&cpuidAddress, 0x80000008, 0);

        if (((UCHAR)cpuidAddress.EAX >= 32) && ((UCHAR)cpuidAddress.EAX < 64))
        {
            identity->PhysicalAddressBits = (UCHAR)cpuidAddress.EAX;
        }
    }

    RtlCopyMemory(vendor, identity->Vendor, sizeof(identity->Vendor));
    vendor[ARRAYSIZE(vendor) - 1] = ANSI_NULL;

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "CPU %hs family 0x%lx model 0x%lx stepping 0x%lx "
                  "max leaf 0x%lx max extended leaf 0x%lx "
                  "physical address bits %lu",
                  vendor,
                  (ULONG)identity->DisplayFamily,
                  (ULONG)identity->DisplayModel,
                  (ULONG)identity->Stepping,
                  identity->MaxBasicLeaf,
                  identity->MaxExtendedLeaf,
                  (ULONG)identity->PhysicalAddressBits);
#endif

#if defined(_M_ARM64)
    switch (_ReadStatusReg(ARM64_ID_AA64MMFR0_EL1) & 0xF)
    {
        case 0:
        {
            KphpHwCpuIdentity.PhysicalAddressBits = 32;
            break;
        }
        case 1:
        {
            KphpHwCpuIdentity.PhysicalAddressBits = 36;
            break;
        }
        case 2:
        {
            KphpHwCpuIdentity.PhysicalAddressBits = 40;
            break;
        }
        case 3:
        {
            KphpHwCpuIdentity.PhysicalAddressBits = 42;
            break;
        }
        case 4:
        {
            KphpHwCpuIdentity.PhysicalAddressBits = 44;
            break;
        }
        case 6:
        {
            KphpHwCpuIdentity.PhysicalAddressBits = 52;
            break;
        }
        default:
        {
            KphpHwCpuIdentity.PhysicalAddressBits = 48;
            break;
        }
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "CPU physical address bits %lu",
                  (ULONG)KphpHwCpuIdentity.PhysicalAddressBits);
#endif

    KphpHwInitializeBoardIdentity();
    KphInitializeHardwarePci();
    KphInitializeHardwareWindow();
    KphInitializeHardwareSuperIo();
    KphInitializeHardwareSmbus();
    KphInitializeHardwareMailbox();
    KphInitializeHardwareAcpi();
}

/**
 * \brief Cleans up the hardware infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardware(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphCleanupHardwareAcpi();
    KphCleanupHardwareMailbox();
    KphCleanupHardwareSmbus();
    KphCleanupHardwareSuperIo();
    KphCleanupHardwarePci();
}

/**
 * \brief Queries the hardware capabilities of the driver.
 *
 * \param[out] Capabilities Receives the hardware capabilities.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwQueryCapabilities(
    _Out_ PKPH_HW_CAPABILITIES Capabilities,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_HW_CAPABILITIES capabilities;
    ULONG64 kinds;
    ULONG64 identityKinds;
    PCKPH_HW_CPU_IDENTITY cpu;
    PKPH_DYN dyn;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&capabilities, sizeof(capabilities));

    capabilities.Version = sizeof(KPH_HW_CAPABILITIES);

#if defined(_M_X64)
    capabilities.Machine = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_ARM64)
    capabilities.Machine = IMAGE_FILE_MACHINE_ARM64;
#else
#error "unsupported architecture"
#endif

    cpu = KphGetHardwareCpuIdentity();

    RtlCopyMemory(capabilities.CpuVendor,
                  cpu->Vendor,
                  sizeof(capabilities.CpuVendor));
    capabilities.DisplayFamily = cpu->DisplayFamily;
    capabilities.DisplayModel = cpu->DisplayModel;
    capabilities.Stepping = cpu->Stepping;

    kinds = 0;

#if defined(_M_X64)
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_MSR_READ));
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_PCI_INDIRECT_READ));
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_SUPERIO_READ));
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_SMBUS_READ));
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_SMBUS_HOST));
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_MAILBOX_TRANSACT));
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_SMU_TABLE_READ));
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_PORT_INDIRECT_READ));
#endif
#if defined(_M_ARM64)
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_SYSREG_READ));
#endif
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_PCI_READ));
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_WINDOW_READ));
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_VSEC_READ));
    SetFlag(kinds, (1ull << KPH_DYN_HW_KIND_ACPI_EVAL));

    identityKinds = KPH_DYN_HW_IMPLEMENTED_IDENTITY_KINDS;
#if !defined(_M_X64)
    ClearFlag(identityKinds, (1ull << KPH_DYN_HW_IDENTITY_CPU));
    ClearFlag(identityKinds, (1ull << KPH_DYN_HW_IDENTITY_CPU_AND_PCI));
    ClearFlag(identityKinds, (1ull << KPH_DYN_HW_IDENTITY_CPU_AND_PCI_CLASS));
    ClearFlag(identityKinds, (1ull << KPH_DYN_HW_IDENTITY_SUPERIO));
#endif
#if !defined(_M_ARM64)
    ClearFlag(identityKinds, (1ull << KPH_DYN_HW_IDENTITY_ARM_CPU));
#endif

    capabilities.ImplementedKinds = kinds;
    capabilities.ImplementedIdentityKinds = identityKinds;
    capabilities.BaseboardHash = KphpHwBoardIdentity.BaseboardHash;
    capabilities.SystemHash = KphpHwBoardIdentity.SystemHash;

    if (KphHwPciAvailable())
    {
        SetFlag(capabilities.Flags, KPH_HW_CAP_PCI_AVAILABLE);
    }

    if (KphHwAcpiAvailable())
    {
        SetFlag(capabilities.Flags, KPH_HW_CAP_ACPI_AVAILABLE);
    }

    dyn = KphReferenceDynData();
    if (dyn)
    {
        if (dyn->Hardware.Section)
        {
            SetFlag(capabilities.Flags, KPH_HW_CAP_SECTION_PRESENT);
        }

        if (dyn->Hardware.CpuTable)
        {
            SetFlag(capabilities.Flags, KPH_HW_CAP_CPU_HAS_ROW);
        }

        if (dyn->Hardware.BoardTable)
        {
            if (dyn->Hardware.BoardSystem)
            {
                SetFlag(capabilities.Flags, KPH_HW_CAP_BOARD_ROW_SYSTEM);
            }
            else
            {
                SetFlag(capabilities.Flags, KPH_HW_CAP_BOARD_ROW_BASEBOARD);
            }
        }

        KphDereferenceObject(dyn);
    }

    return KphCopyToMode(Capabilities,
                         &capabilities,
                         sizeof(KPH_HW_CAPABILITIES),
                         AccessMode);
}

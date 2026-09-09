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

#include "agenttools.h"
#include <nvme.h>

// SMART attributes arrive as a 512-byte vendor block: a two-byte header then up to thirty
// twelve-byte records. The fields are read at their byte offsets because a struct of these types is
// fourteen bytes once aligned.
#define AT_SMART_HEADER_SIZE 2
#define AT_SMART_ATTRIBUTE_SIZE 12
#define AT_SMART_ATTRIBUTE_COUNT 30
#define AT_SMART_OFFSET_ID 0
#define AT_SMART_OFFSET_FLAGS 1
#define AT_SMART_OFFSET_CURRENT 3
#define AT_SMART_OFFSET_WORST 4
#define AT_SMART_OFFSET_RAW 5

PCSTR AtpStorageBusTypeString(
    _In_ STORAGE_BUS_TYPE BusType
    )
{
    switch (BusType)
    {
    case BusTypeScsi:
        return "scsi";
    case BusTypeAtapi:
        return "atapi";
    case BusTypeAta:
        return "ata";
    case BusType1394:
        return "ieee1394";
    case BusTypeSsa:
        return "ssa";
    case BusTypeFibre:
        return "fibre_channel";
    case BusTypeUsb:
        return "usb";
    case BusTypeRAID:
        return "raid";
    case BusTypeiScsi:
        return "iscsi";
    case BusTypeSas:
        return "sas";
    case BusTypeSata:
        return "sata";
    case BusTypeSd:
        return "sd";
    case BusTypeMmc:
        return "mmc";
    case BusTypeVirtual:
        return "virtual";
    case BusTypeFileBackedVirtual:
        return "file_backed_virtual";
    case BusTypeSpaces:
        return "storage_spaces";
    case BusTypeNvme:
        return "nvme";
    case BusTypeSCM:
        return "scm";
    case BusTypeUfs:
        return "ufs";
    }

    return "unknown";
}

PPH_LIST AtpEnumerateDiskPaths(
    VOID
    )
{
    PPH_LIST paths;
    ULONG objectCount = 0;
    const DEV_OBJECT* objects = NULL;
    const DEVPROP_FILTER_EXPRESSION filter[] =
    {
        { DEVPROP_OPERATOR_EQUALS, {{ DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL }, DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_DEVINTERFACE_DISK } }
    };
    ULONG i;

    if (HR_FAILED(PhDevGetObjects(
        DevObjectTypeDeviceInterface,
        DevQueryFlagNone,
        0,
        NULL,
        RTL_NUMBER_OF(filter),
        filter,
        &objectCount,
        &objects
        )))
    {
        return NULL;
    }

    paths = PhCreateList(objectCount);

    for (i = 0; i < objectCount; i++)
    {
        PPH_STRING path;

        if (!objects[i].pszObjectId)
            continue;

        // The interface path is a Win32 \\?\ path; the second character becomes a question mark
        // to make it the \??\ object path the native open wants.
        path = PhCreateString(objects[i].pszObjectId);

        if (path->Length >= 4 * sizeof(WCHAR) && path->Buffer[1] == OBJ_NAME_PATH_SEPARATOR)
            path->Buffer[1] = L'?';

        PhAddItemList(paths, path);
    }

    PhDevFreeObjects(objectCount, objects);

    return paths;
}

NTSTATUS AtpOpenDisk(
    _Out_ PHANDLE DeviceHandle,
    _In_ PPH_STRING Path
    )
{
    return PhCreateFile(
        DeviceHandle,
        &Path->sr,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );
}

ULONG AtpQueryDiskNumber(
    _In_ HANDLE DeviceHandle
    )
{
    STORAGE_DEVICE_NUMBER deviceNumber;

    memset(&deviceNumber, 0, sizeof(STORAGE_DEVICE_NUMBER));

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_STORAGE_GET_DEVICE_NUMBER,
        NULL,
        0,
        &deviceNumber,
        sizeof(STORAGE_DEVICE_NUMBER),
        NULL
        )))
    {
        return deviceNumber.DeviceNumber;
    }

    return ULONG_MAX;
}

PSTORAGE_DEVICE_DESCRIPTOR AtpQueryDiskDescriptor(
    _In_ HANDLE DeviceHandle
    )
{
    STORAGE_PROPERTY_QUERY query;
    STORAGE_DESCRIPTOR_HEADER header;
    PSTORAGE_DEVICE_DESCRIPTOR descriptor;

    memset(&query, 0, sizeof(STORAGE_PROPERTY_QUERY));
    query.QueryType = PropertyStandardQuery;
    query.PropertyId = StorageDeviceProperty;

    memset(&header, 0, sizeof(STORAGE_DESCRIPTOR_HEADER));

    if (!NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &query,
        sizeof(query),
        &header,
        sizeof(header),
        NULL
        )))
    {
        return NULL;
    }

    if (header.Size < sizeof(STORAGE_DEVICE_DESCRIPTOR))
        return NULL;

    descriptor = PhAllocateZero(header.Size);

    if (!NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &query,
        sizeof(query),
        descriptor,
        header.Size,
        NULL
        )))
    {
        PhFree(descriptor);
        return NULL;
    }

    return descriptor;
}

VOID AtpAddDescriptorString(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PSTORAGE_DEVICE_DESCRIPTOR Descriptor,
    _In_ ULONG Offset
    )
{
    PPH_STRING string;
    PPH_STRING trimmed;

    if (!Offset)
    {
        AtJsonAddNull(Object, Key);
        return;
    }

    string = PhConvertUtf8ToUtf16(PTR_ADD_OFFSET(Descriptor, Offset));
    trimmed = PhTrimStringZ(&string->sr, 0, L" \t\r\n");

    AtJsonAddString(Object, Key, trimmed->Length ? trimmed : NULL);

    PhDereferenceObject(trimmed);
    PhDereferenceObject(string);
}

VOID AtpAddDiskModel(
    _In_ PVOID Row,
    _In_ HANDLE DeviceHandle
    )
{
    PSTORAGE_DEVICE_DESCRIPTOR descriptor;

    if (descriptor = AtpQueryDiskDescriptor(DeviceHandle))
    {
        AtpAddDescriptorString(Row, "model", descriptor, descriptor->ProductIdOffset);
        PhFree(descriptor);
    }
    else
    {
        AtJsonAddNull(Row, "model");
    }
}

VOID AtpAddDiskIdentity(
    _In_ PVOID Row,
    _In_ HANDLE DeviceHandle
    )
{
    PSTORAGE_DEVICE_DESCRIPTOR descriptor;
    DISK_GEOMETRY_EX geometry;

    if (descriptor = AtpQueryDiskDescriptor(DeviceHandle))
    {
        AtpAddDescriptorString(Row, "vendor", descriptor, descriptor->VendorIdOffset);
        AtpAddDescriptorString(Row, "model", descriptor, descriptor->ProductIdOffset);
        AtpAddDescriptorString(Row, "revision", descriptor, descriptor->ProductRevisionOffset);
        AtpAddDescriptorString(Row, "serial_number", descriptor, descriptor->SerialNumberOffset);
        PhAddJsonObject(Row, "bus_type", AtpStorageBusTypeString(descriptor->BusType));
        PhAddJsonObjectBoolean(Row, "removable", !!descriptor->RemovableMedia);
        PhAddJsonObjectBoolean(Row, "command_queueing", !!descriptor->CommandQueueing);
        PhFree(descriptor);
    }
    else
    {
        AtJsonAddNull(Row, "vendor");
        AtJsonAddNull(Row, "model");
        AtJsonAddNull(Row, "revision");
        AtJsonAddNull(Row, "serial_number");
        AtJsonAddNull(Row, "bus_type");
        AtJsonAddNull(Row, "removable");
        AtJsonAddNull(Row, "command_queueing");
    }

    memset(&geometry, 0, sizeof(DISK_GEOMETRY_EX));

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
        NULL,
        0,
        &geometry,
        sizeof(DISK_GEOMETRY_EX),
        NULL
        )))
    {
        PhAddJsonObjectUInt64(Row, "size_bytes", geometry.DiskSize.QuadPart);
        PhAddJsonObjectUInt64(Row, "bytes_per_sector", geometry.Geometry.BytesPerSector);
        PhAddJsonObjectUInt64(Row, "sectors_per_track", geometry.Geometry.SectorsPerTrack);
        PhAddJsonObjectUInt64(Row, "tracks_per_cylinder", geometry.Geometry.TracksPerCylinder);
        PhAddJsonObjectUInt64(Row, "cylinders", geometry.Geometry.Cylinders.QuadPart);
    }
    else
    {
        AtJsonAddNull(Row, "size_bytes");
        AtJsonAddNull(Row, "bytes_per_sector");
        AtJsonAddNull(Row, "sectors_per_track");
        AtJsonAddNull(Row, "tracks_per_cylinder");
        AtJsonAddNull(Row, "cylinders");
    }
}

BOOLEAN AtpAddDiskPerformance(
    _In_ PVOID Row,
    _In_ HANDLE DeviceHandle
    )
{
    DISK_PERFORMANCE performance;

    memset(&performance, 0, sizeof(DISK_PERFORMANCE));

    if (!NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_DISK_PERFORMANCE,
        NULL,
        0,
        &performance,
        sizeof(DISK_PERFORMANCE),
        NULL
        )))
    {
        return FALSE;
    }

    PhAddJsonObjectUInt64(Row, "bytes_read", performance.BytesRead.QuadPart);
    PhAddJsonObjectUInt64(Row, "bytes_written", performance.BytesWritten.QuadPart);
    PhAddJsonObjectUInt64(Row, "read_count", performance.ReadCount);
    PhAddJsonObjectUInt64(Row, "write_count", performance.WriteCount);
    PhAddJsonObjectUInt64(Row, "read_time_100ns", performance.ReadTime.QuadPart);
    PhAddJsonObjectUInt64(Row, "write_time_100ns", performance.WriteTime.QuadPart);
    PhAddJsonObjectUInt64(Row, "idle_time_100ns", performance.IdleTime.QuadPart);
    PhAddJsonObjectUInt64(Row, "query_time_100ns", performance.QueryTime.QuadPart);
    PhAddJsonObjectUInt64(Row, "split_count", performance.SplitCount);
    PhAddJsonObjectUInt64(Row, "queue_depth", performance.QueueDepth);

    return TRUE;
}

VOID AtpAddSmartAttributes(
    _In_ PVOID Row,
    _In_ HANDLE DeviceHandle
    )
{
    STORAGE_PREDICT_FAILURE predictFailure;
    PVOID attributes;
    UCHAR i;

    memset(&predictFailure, 0, sizeof(STORAGE_PREDICT_FAILURE));

    if (!NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_STORAGE_PREDICT_FAILURE,
        NULL,
        0,
        &predictFailure,
        sizeof(STORAGE_PREDICT_FAILURE),
        NULL
        )))
    {
        AtJsonAddNull(Row, "predicted_failure");
        AtJsonAddNull(Row, "smart_attributes");
        return;
    }

    PhAddJsonObjectBoolean(Row, "predicted_failure", !!predictFailure.PredictFailure);

    attributes = PhCreateJsonArray();

    for (i = 0; i < AT_SMART_ATTRIBUTE_COUNT; i++)
    {
        const UCHAR* attribute;
        PVOID entry;
        UCHAR id;
        USHORT flags;
        ULONG64 raw;
        ULONG j;

        attribute = PTR_ADD_OFFSET(predictFailure.VendorSpecific, AT_SMART_HEADER_SIZE + i * AT_SMART_ATTRIBUTE_SIZE);
        id = attribute[AT_SMART_OFFSET_ID];

        // 0x00, 0xFE and 0xFF are not attribute ids. The table has gaps, so one of them is not
        // the end of it.
        if (id == 0x00 || id == 0xFE || id == 0xFF)
            continue;

        flags = (USHORT)(attribute[AT_SMART_OFFSET_FLAGS] | (attribute[AT_SMART_OFFSET_FLAGS + 1] << 8));

        // The raw value is six bytes little-endian. Reading only four of them, which is the
        // obvious mistake, silently truncates power-on hours and written-bytes counters.
        raw = 0;

        for (j = 0; j < 6; j++)
            raw |= (ULONG64)attribute[AT_SMART_OFFSET_RAW + j] << (j * 8);

        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "id", id);
        PhAddJsonObjectUInt64(entry, "current_value", attribute[AT_SMART_OFFSET_CURRENT]);
        PhAddJsonObjectUInt64(entry, "worst_value", attribute[AT_SMART_OFFSET_WORST]);
        PhAddJsonObjectUInt64(entry, "raw_value", raw);
        AtJsonAddHex(entry, "flags", flags);
        // Only the first two flag bits are defined by the specification; the rest are vendor
        // specific and stay in the raw flags word rather than being given invented names.
        PhAddJsonObjectBoolean(entry, "pre_failure", !!(flags & 0x1));
        PhAddJsonObjectBoolean(entry, "online_collection", !!(flags & 0x2));

        PhAddJsonArrayObject(attributes, entry);
    }

    PhAddJsonObjectValue(Row, "smart_attributes", attributes);
}

ULONG64 AtpNvmeCounter(
    _In_reads_(16) const UCHAR* Value
    )
{
    ULONG64 result = 0;
    ULONG i;

    for (i = 0; i < 8; i++)
        result |= (ULONG64)Value[i] << (i * 8);

    return result;
}

VOID AtpAddNvmeHealth(
    _In_ PVOID Row,
    _In_ HANDLE DeviceHandle
    )
{
    UCHAR buffer[FIELD_OFFSET(STORAGE_PROPERTY_QUERY, AdditionalParameters) + sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + sizeof(NVME_HEALTH_INFO_LOG)];
    PSTORAGE_PROPERTY_QUERY query;
    PSTORAGE_PROTOCOL_SPECIFIC_DATA protocolData;
    PSTORAGE_PROTOCOL_DATA_DESCRIPTOR descriptor;
    PNVME_HEALTH_INFO_LOG health;
    ULONG returnedLength = 0;
    PVOID entry;
    ULONG temperature;

    memset(buffer, 0, sizeof(buffer));

    query = (PSTORAGE_PROPERTY_QUERY)buffer;
    query->PropertyId = StorageDeviceProtocolSpecificProperty;
    query->QueryType = PropertyStandardQuery;

    protocolData = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;
    protocolData->ProtocolType = ProtocolTypeNvme;
    protocolData->DataType = NVMeDataTypeLogPage;
    protocolData->ProtocolDataRequestValue = NVME_LOG_PAGE_HEALTH_INFO;
    protocolData->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
    protocolData->ProtocolDataLength = sizeof(NVME_HEALTH_INFO_LOG);

    if (!NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_STORAGE_QUERY_PROPERTY,
        buffer,
        sizeof(buffer),
        buffer,
        sizeof(buffer),
        &returnedLength
        )) || returnedLength < sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))
    {
        AtJsonAddNull(Row, "nvme");
        return;
    }

    descriptor = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;

    if (descriptor->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR) ||
        descriptor->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))
    {
        AtJsonAddNull(Row, "nvme");
        return;
    }

    protocolData = &descriptor->ProtocolSpecificData;

    if (protocolData->ProtocolDataOffset < sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) ||
        protocolData->ProtocolDataLength < sizeof(NVME_HEALTH_INFO_LOG))
    {
        AtJsonAddNull(Row, "nvme");
        return;
    }

    health = PTR_ADD_OFFSET(protocolData, protocolData->ProtocolDataOffset);
    entry = PhCreateJsonObject();

    // The log reports temperature in Kelvin; reported raw it reads as a few hundred degrees.
    temperature = health->Temperature[0] | ((ULONG)health->Temperature[1] << 8);

    if (temperature)
        PhAddJsonObjectInt64(entry, "temperature_celsius", (LONG)temperature - 273);
    else
        AtJsonAddNull(entry, "temperature_celsius");

    PhAddJsonObjectUInt64(entry, "available_spare_percent", health->AvailableSpare);
    PhAddJsonObjectUInt64(entry, "available_spare_threshold_percent", health->AvailableSpareThreshold);
    PhAddJsonObjectUInt64(entry, "percentage_used", health->PercentageUsed);
    PhAddJsonObjectUInt64(entry, "power_on_hours", AtpNvmeCounter(health->PowerOnHours));
    PhAddJsonObjectUInt64(entry, "power_cycles", AtpNvmeCounter(health->PowerCycle));
    PhAddJsonObjectUInt64(entry, "unsafe_shutdowns", AtpNvmeCounter(health->UnsafeShutdowns));
    PhAddJsonObjectUInt64(entry, "media_errors", AtpNvmeCounter(health->MediaErrors));
    PhAddJsonObjectUInt64(entry, "error_log_entries", AtpNvmeCounter(health->ErrorInfoLogEntryCount));

    // Data units are 512-byte units counted in thousands, so neither factor can be dropped.
    PhAddJsonObjectUInt64(entry, "data_read_bytes", AtpNvmeCounter(health->DataUnitRead) * 1000 * 512);
    PhAddJsonObjectUInt64(entry, "data_written_bytes", AtpNvmeCounter(health->DataUnitWritten) * 1000 * 512);

    PhAddJsonObjectBoolean(entry, "spare_below_threshold", !!health->CriticalWarning.AvailableSpaceLow);
    PhAddJsonObjectBoolean(entry, "temperature_threshold_exceeded", !!health->CriticalWarning.TemperatureThreshold);
    PhAddJsonObjectBoolean(entry, "reliability_degraded", !!health->CriticalWarning.ReliabilityDegraded);
    PhAddJsonObjectBoolean(entry, "read_only_mode", !!health->CriticalWarning.ReadOnly);
    PhAddJsonObjectBoolean(entry, "volatile_memory_backup_failed", !!health->CriticalWarning.VolatileMemoryBackupDeviceFailed);

    PhAddJsonObjectValue(Row, "nvme", entry);
}

typedef enum _AT_DISK_TOOL
{
    AtDiskToolPerformance,
    AtDiskToolIdentity,
    AtDiskToolHealth
} AT_DISK_TOOL;

VOID AtpEnumerateDisks(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result,
    _In_ AT_DISK_TOOL Tool
    )
{
    PPH_LIST paths;
    AT_ROWS rows;
    PVOID structured;
    ULONG64 diskNumber;
    BOOLEAN haveDiskNumber;
    ULONG i;

    haveDiskNumber = AtGetArgumentUInt64(Call->Arguments, "disk_number", &diskNumber);

    if (!(paths = AtpEnumerateDiskPaths()))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"Enumerating the disk devices failed.");
        return;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < paths->Count; i++)
    {
        PPH_STRING path = paths->Items[i];
        HANDLE deviceHandle;
        PVOID row;
        ULONG number;

        if (!NT_SUCCESS(AtpOpenDisk(&deviceHandle, path)))
            continue;

        number = AtpQueryDiskNumber(deviceHandle);

        if (haveDiskNumber && number != (ULONG)diskNumber)
        {
            NtClose(deviceHandle);
            continue;
        }

        row = PhCreateJsonObject();

        if (number != ULONG_MAX)
            PhAddJsonObjectUInt64(row, "disk_number", number);
        else
            AtJsonAddNull(row, "disk_number");

        switch (Tool)
        {
        case AtDiskToolPerformance:
            AtpAddDiskModel(row, deviceHandle);

            if (!AtpAddDiskPerformance(row, deviceHandle))
            {
                // Counters the storage stack would not give up say nothing about the disk, and a
                // row of zeroes would say the disk is idle.
                PhFreeJsonObject(row);
                NtClose(deviceHandle);
                continue;
            }
            break;
        case AtDiskToolIdentity:
            AtJsonAddString(row, "device_path", path);
            AtpAddDiskIdentity(row, deviceHandle);
            break;
        case AtDiskToolHealth:
            AtpAddDiskModel(row, deviceHandle);
            AtpAddSmartAttributes(row, deviceHandle);
            AtpAddNvmeHealth(row, deviceHandle);
            break;
        }

        AtAddRow(&rows, row);
        NtClose(deviceHandle);
    }

    AtAddRows(structured, "disks", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&rows);
    PhDereferenceObjects(paths->Items, paths->Count);
    PhDereferenceObject(paths);
}

VOID AtDiskInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionGetDiskPerformance:
        AtpEnumerateDisks(Call, Result, AtDiskToolPerformance);
        break;
    case AtActionGetDiskIdentity:
        AtpEnumerateDisks(Call, Result, AtDiskToolIdentity);
        break;
    case AtActionGetDiskHealth:
        AtpEnumerateDisks(Call, Result, AtDiskToolHealth);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}

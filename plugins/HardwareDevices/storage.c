/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2022
 *
 */

#include "devices.h"
#include <ntdddisk.h>

PPH_STRING DiskDriveQueryDosMountPoints(
    _In_ ULONG DeviceNumber
    )
{
    ULONG deviceMap = 0;
    WCHAR deviceNameBuffer[7] = L"\\??\\?:";
    PH_STRINGREF deviceName;
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, DOS_MAX_PATH_LENGTH);

    PhGetProcessDeviceMap(NtCurrentProcess(), &deviceMap);

    for (INT i = 0; i < 0x1A; i++)
    {
        HANDLE deviceHandle;

        if (deviceMap)
        {
            if (!(deviceMap & (0x1 << i)))
                continue;
        }

        deviceNameBuffer[4] = (WCHAR)('A' + i);
        deviceName.Buffer = deviceNameBuffer;
        deviceName.Length = 6 * sizeof(WCHAR);

        if (NT_SUCCESS(PhCreateFile(
            &deviceHandle,
            &deviceName,
            FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            ULONG deviceNumber = ULONG_MAX; // Note: Do not initialize to zero.
            DEVICE_TYPE deviceType = 0;

            if (NT_SUCCESS(DiskDriveQueryDeviceTypeAndNumber(
                deviceHandle,
                &deviceNumber,
                &deviceType
                )))
            {
                // Note: The device number is not locally unique. If there are bug reports about incorrect mount points
                // for disks then remove the check for FILE_DEVICE_CD_ROM and replace it with deviceType==FILE_DEVICE_DISK (dmex)
                if (deviceNumber == DeviceNumber && deviceType != FILE_DEVICE_CD_ROM)
                {
                    PH_FORMAT format[2];
                    SIZE_T returnLength;
                    WCHAR formatBuffer[0x100];

                    PhInitFormatC(&format[0], deviceNameBuffer[4]);
                    PhInitFormatS(&format[1], L": ");

                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
                    {
                        PhAppendStringBuilderEx(&stringBuilder, formatBuffer, returnLength - sizeof(UNICODE_NULL));
                    }
                    else
                    {
                        PhAppendFormatStringBuilder(&stringBuilder, L"%c: ", deviceName.Buffer[4]);
                    }
                }
            }

            NtClose(deviceHandle);
        }
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_LIST DiskDriveQueryMountPointHandles(
    _In_ ULONG DeviceNumber
    )
{
    PH_STRINGREF deviceName;
    WCHAR deviceNameBuffer[7] = L"\\??\\?:";
    ULONG deviceMap = 0;
    PPH_LIST deviceList;

    PhGetProcessDeviceMap(NtCurrentProcess(), &deviceMap);

    deviceList = PhCreateList(2);

    for (INT i = 0; i < 26; i++)
    {
        HANDLE deviceHandle;

        if (deviceMap)
        {
            if (!(deviceMap & (0x1 << i)))
                continue;
        }

        deviceNameBuffer[4] = (WCHAR)('A' + i);
        deviceName.Buffer = deviceNameBuffer;
        deviceName.Length = 6 * sizeof(WCHAR);

        if (NT_SUCCESS(PhCreateFile(
            &deviceHandle,
            &deviceName,
            PhGetOwnTokenAttributes().Elevated ? FILE_GENERIC_READ : FILE_READ_ATTRIBUTES | FILE_TRAVERSE | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            ULONG deviceNumber = ULONG_MAX;
            DEVICE_TYPE deviceType = 0;

            if (NT_SUCCESS(DiskDriveQueryDeviceTypeAndNumber(
                deviceHandle,
                &deviceNumber,
                &deviceType
                )))
            {
                // Note: The device number is not locally unique. If there are bug reports about incorrect mount points
                // for disks then remove the check for FILE_DEVICE_CD_ROM and replace it with deviceType==FILE_DEVICE_DISK (dmex)
                if (deviceNumber == DeviceNumber && deviceType != FILE_DEVICE_CD_ROM)
                {
                    PDISK_HANDLE_ENTRY entry;

                    entry = PhAllocateZero(sizeof(DISK_HANDLE_ENTRY));
                    entry->DeviceLetter = deviceNameBuffer[4];
                    entry->DeviceHandle = deviceHandle;

                    PhAddItemList(deviceList, entry);
                }
                else
                {
                    NtClose(deviceHandle);
                }
            }
            else
            {
                NtClose(deviceHandle);
            }
        }
    }

    return deviceList;
}

_Success_(return)
BOOLEAN DiskDriveQueryDeviceInformation(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PPH_STRING* DiskVendor,
    _Out_opt_ PPH_STRING* DiskModel,
    _Out_opt_ PPH_STRING* DiskRevision,
    _Out_opt_ PPH_STRING* DiskSerial
    )
{
    ULONG bufferLength;
    STORAGE_PROPERTY_QUERY query;
    PSTORAGE_DESCRIPTOR_HEADER buffer;

    query.QueryType = PropertyStandardQuery;
    query.PropertyId = StorageDeviceProperty;

    bufferLength = sizeof(STORAGE_DESCRIPTOR_HEADER);
    buffer = PhAllocate(bufferLength);
    memset(buffer, 0, bufferLength);

    if (!NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &query,
        sizeof(query),
        buffer,
        bufferLength,
        NULL
        )))
    {
        PhFree(buffer);
        return FALSE;
    }

    bufferLength = buffer->Size;
    buffer = PhReAllocate(buffer, bufferLength);
    memset(buffer, 0, bufferLength);

    if (!NT_SUCCESS(PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &query,
        sizeof(query),
        buffer,
        bufferLength,
        NULL
        )))
    {
        PhFree(buffer);
        return FALSE;
    }

    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)buffer;

    if (DiskVendor)
    {
        if (deviceDescriptor->VendorIdOffset != 0)
        {
            PPH_STRING string;

            string = PhConvertMultiByteToUtf16(PTR_ADD_OFFSET(deviceDescriptor, deviceDescriptor->VendorIdOffset));
            *DiskVendor = TrimString(string);
            PhDereferenceObject(string);
        }
        else
        {
            *DiskVendor = PhReferenceEmptyString();
        }
    }

    if (DiskModel)
    {
        if (deviceDescriptor->ProductIdOffset != 0)
        {
            PPH_STRING string;

            string = PhConvertMultiByteToUtf16(PTR_ADD_OFFSET(deviceDescriptor, deviceDescriptor->ProductIdOffset));
            *DiskModel = TrimString(string);
            PhDereferenceObject(string);
        }
        else
        {
            *DiskModel = PhReferenceEmptyString();
        }
    }

    if (DiskRevision)
    {
        if (deviceDescriptor->ProductRevisionOffset != 0)
        {
            PPH_STRING string;

            string = PhConvertMultiByteToUtf16(PTR_ADD_OFFSET(deviceDescriptor, deviceDescriptor->ProductRevisionOffset));
            *DiskRevision = TrimString(string);
            PhDereferenceObject(string);
        }
        else
        {
            *DiskRevision = PhReferenceEmptyString();
        }
    }

    if (DiskSerial)
    {
        if (deviceDescriptor->SerialNumberOffset != 0)
        {
            PPH_STRING string;

            string = PhConvertMultiByteToUtf16(PTR_ADD_OFFSET(deviceDescriptor, deviceDescriptor->SerialNumberOffset));
            *DiskSerial = TrimString(string);
            PhDereferenceObject(string);
        }
        else
        {
            *DiskSerial = PhReferenceEmptyString();
        }
    }

    PhFree(buffer);

    return TRUE;
}

NTSTATUS DiskDriveQueryDeviceTypeAndNumber(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PULONG DeviceNumber,
    _Out_opt_ DEVICE_TYPE* DeviceType
    )
{
    NTSTATUS status;
    STORAGE_DEVICE_NUMBER result;

    memset(&result, 0, sizeof(STORAGE_DEVICE_NUMBER));

    status = PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_STORAGE_GET_DEVICE_NUMBER,
        NULL,
        0,
        &result,
        sizeof(result),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (DeviceType)
        {
            *DeviceType = result.DeviceType;
        }

        if (DeviceNumber)
        {
            *DeviceNumber = result.DeviceNumber;
        }
    }

    return status;
}

NTSTATUS DiskDriveQueryStatistics(
    _In_ HANDLE DeviceHandle,
    _Out_ PDISK_PERFORMANCE Info
    )
{
    NTSTATUS status;
    DISK_PERFORMANCE result;

    memset(&result, 0, sizeof(DISK_PERFORMANCE));

    status = PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_DISK_PERFORMANCE, // IOCTL_DISK_GET_PERFORMANCE_INFO (ntdddisk.h)
        NULL,
        0,
        &result,
        sizeof(result),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *Info = result;
    }

    return status;
}

PPH_STRING DiskDriveQueryGeometry(
    _In_ HANDLE DeviceHandle
    )
{
    {
        DISK_GEOMETRY_EX result;

        memset(&result, 0, sizeof(DISK_GEOMETRY_EX));

        if (NT_SUCCESS(PhDeviceIoControlFile(
            DeviceHandle,
            IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
            NULL,
            0,
            &result,
            sizeof(result),
            NULL
            )))
        {
            ULONG64 formattedSize = result.Geometry.Cylinders.QuadPart * result.Geometry.TracksPerCylinder * result.Geometry.SectorsPerTrack * result.Geometry.BytesPerSector;
            ULONG64 capacitySize = result.DiskSize.QuadPart;

            return PhFormatSize(capacitySize, ULONG_MAX);
        }
    }

    {
        DISK_GEOMETRY result;

        memset(&result, 0, sizeof(DISK_GEOMETRY));

        if (NT_SUCCESS(PhDeviceIoControlFile(
            DeviceHandle,
            IOCTL_DISK_GET_DRIVE_GEOMETRY,
            NULL,
            0,
            &result,
            sizeof(result),
            NULL
            )))
        {
            return PhFormatSize(result.Cylinders.QuadPart * result.TracksPerCylinder * result.SectorsPerTrack * result.BytesPerSector, ULONG_MAX);
        }
    }

    {
        GET_LENGTH_INFORMATION result;

        memset(&result, 0, sizeof(GET_LENGTH_INFORMATION));

        if (NT_SUCCESS(PhDeviceIoControlFile(
            DeviceHandle,
            IOCTL_DISK_GET_LENGTH_INFO, // requires FILE_GENERIC_READ (dmex)
            NULL,
            0,
            &result,
            sizeof(result),
            NULL
            )))
        {
            return PhFormatSize(result.Length.QuadPart, ULONG_MAX);
        }
    }

    {
        STORAGE_READ_CAPACITY result;

        memset(&result, 0, sizeof(STORAGE_READ_CAPACITY));
        result.Version = sizeof(STORAGE_READ_CAPACITY);

        if (NT_SUCCESS(PhDeviceIoControlFile(
            DeviceHandle,
            IOCTL_STORAGE_READ_CAPACITY, // requires FILE_GENERIC_READ (dmex)
            NULL,
            0,
            &result,
            sizeof(result),
            NULL
            )))
        {
            if (RTL_CONTAINS_FIELD(&result, result.Size, DiskLength))
            {
                return PhFormatSize(result.DiskLength.QuadPart, ULONG_MAX);
            }
        }
    }

    return PhReferenceEmptyString();
}

NTSTATUS DiskDriveQueryImminentFailure(
    _In_ HANDLE DeviceHandle,
    _Out_ PPH_LIST* DiskSmartAttributes
    )
{
    NTSTATUS status;
    STORAGE_PREDICT_FAILURE storagePredictFailure;

    memset(&storagePredictFailure, 0, sizeof(STORAGE_PREDICT_FAILURE));

    // * IOCTL_STORAGE_PREDICT_FAILURE returns an opaque 512-byte vendor-specific information block, which
    //      in all cases contains SMART attribute information (2 bytes header + 12 bytes each attribute).
    // * This works without admin rights but doesn't support other features like logs and self-tests.
    // * It works for (S)ATA devices but not for USB.

    status = PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_STORAGE_PREDICT_FAILURE,
        NULL,
        0,
        &storagePredictFailure,
        sizeof(storagePredictFailure),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PPH_LIST diskAttributeList;
        //USHORT structureVersion = (USHORT)(storagePredictFailure.VendorSpecific[0] * 256 + storagePredictFailure.VendorSpecific[1]);
        //USHORT majorVersion = HIBYTE(structureVersion);
        //USHORT minorVersion = LOBYTE(structureVersion);
        //TODO: include storagePredictFailure.PredictFailure;

        diskAttributeList = PhCreateList(30);

        //An attribute is a one-byte value ranging from 1 to 253 (FDh).
        //The initial default value is 100 (64h).
        //The value and the intertretation of the value are vendor-specific.
        //Attribute values are read-only to the host.
        //A device may report up to 30 attributes to the host.
        //Values of 00h, FEh and FFh are invalid.
        //When attribute values are updated by the device depends on the specific attribute. Some are updated as
        //the disk operates, some are only updated during SMART self-tests, or at special events like power-on or
        //unloading the heads of a disk drive, etc

        //Each attribute may have an associated threshhold. When the value exceeds the threshhold, the attribute
        //triggers a SMART 'threshhold exceeded' event. This event indicates that either the disk is expected to fail
        //in less than 24 hours or it has exceeded its design or usage lifetime.
        //When an attribute value is greater than or equal to the threshhold, the threshhold is considered to be
        //exceeded. A flag is set indicating that failure is likely.
        //There is no standard way for a host to read or change attribute threshholds.
        //See the SMART(RETURN STATUS) command for information about how a device reports that a
        //threshhold has been exceeded.
        // TODO: Query Threshholds.

        for (UCHAR i = 0; i < 30; ++i)
        {
            PSMART_ATTRIBUTE attribute = PTR_ADD_OFFSET(storagePredictFailure.VendorSpecific, i * sizeof(SMART_ATTRIBUTE) + SMART_HEADER_SIZE);

            // Attribute values 0x00, 0xFE, 0xFF are invalid.
            // There is no requirement that attributes be in any particular order.
            if (
                attribute->Id != 0x00 &&
                attribute->Id != 0xFE &&
                attribute->Id != 0xFF
                )
            {
                PSMART_ATTRIBUTES info;

                info = PhAllocateZero(sizeof(SMART_ATTRIBUTES));
                info->AttributeId = attribute->Id;
                info->CurrentValue = attribute->CurrentValue;
                info->WorstValue = attribute->WorstValue;

                // TODO: These flag offsets might be off-by-one.
                info->Advisory = (attribute->Flags & 0x1) == 0x0;
                info->FailureImminent = (attribute->Flags & 0x1) == 0x1;
                info->OnlineDataCollection = (attribute->Flags & 0x2) == 0x2;
                info->Performance = (attribute->Flags & 0x3) == 0x3;
                info->ErrorRate = (attribute->Flags & 0x4) == 0x4;
                info->EventCount = (attribute->Flags & 0x5) == 0x5;
                info->SelfPreserving = (attribute->Flags & 0x6) == 0x6;

                info->RawValue = MAKELONG(
                    MAKEWORD(attribute->RawValue[0], attribute->RawValue[1]),
                    MAKEWORD(attribute->RawValue[2], attribute->RawValue[3])
                    );
                // Missing 2 raw values.

                PhAddItemList(diskAttributeList, info);
            }
        }

        *DiskSmartAttributes = diskAttributeList;
    }

    return status;
}

_Success_(return)
BOOLEAN DiskDriveQueryFileSystemInfo(
    _In_ HANDLE DosDeviceHandle,
    _Out_ USHORT* FileSystemType,
    _Out_ PVOID* FileSystemStatistics
    )
{
    ULONG bufferLength;
    PFILESYSTEM_STATISTICS buffer;

    bufferLength = sizeof(FILESYSTEM_STATISTICS);
    buffer = PhAllocate(bufferLength);
    memset(buffer, 0, bufferLength);

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DosDeviceHandle,
        FSCTL_FILESYSTEM_GET_STATISTICS, // FSCTL_FILESYSTEM_GET_STATISTICS_EX
        NULL,
        0,
        buffer,
        bufferLength,
        NULL
        )))
    {
        PhFree(buffer);
        return FALSE;
    }

    switch (buffer->FileSystemType)
    {
    case FILESYSTEM_STATISTICS_TYPE_NTFS:
    case FILESYSTEM_STATISTICS_TYPE_REFS: // ReFS uses the same statistics as NTFS.
        {
            bufferLength = sizeof(NTFS_FILESYSTEM_STATISTICS) * 64 * PhGetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
            buffer = PhReAllocate(buffer, bufferLength);
            memset(buffer, 0, bufferLength);
        }
        break;
    case FILESYSTEM_STATISTICS_TYPE_FAT:
        {
            bufferLength = sizeof(FAT_FILESYSTEM_STATISTICS) * 64 * PhGetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
            buffer = PhReAllocate(buffer, bufferLength);
            memset(buffer, 0, bufferLength);
        }
        break;
    case FILESYSTEM_STATISTICS_TYPE_EXFAT:
        {
            bufferLength = sizeof(EXFAT_FILESYSTEM_STATISTICS) * 64 * PhGetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
            buffer = PhReAllocate(buffer, bufferLength);
            memset(buffer, 0, bufferLength);
        }
        break;
    }

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DosDeviceHandle,
        FSCTL_FILESYSTEM_GET_STATISTICS,
        NULL,
        0,
        buffer,
        bufferLength,
        NULL
        )))
    {
        *FileSystemType = buffer->FileSystemType;
        *FileSystemStatistics = buffer;
        return TRUE;
    }

    PhFree(buffer);
    return FALSE;
}

_Success_(return)
BOOLEAN DiskDriveQueryNtfsVolumeInfo(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PNTFS_VOLUME_INFO VolumeInfo
    )
{
    NTFS_VOLUME_INFO result;

    memset(&result, 0, sizeof(NTFS_VOLUME_INFO));

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DosDeviceHandle,
        FSCTL_GET_NTFS_VOLUME_DATA,
        NULL,
        0,
        &result,
        sizeof(result),
        NULL
        )))
    {
        *VolumeInfo = result;
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN DiskDriveQueryRefsVolumeInfo(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PREFS_VOLUME_DATA_BUFFER VolumeInfo
    )
{
    REFS_VOLUME_DATA_BUFFER result;

    memset(&result, 0, sizeof(REFS_VOLUME_DATA_BUFFER));

    if (NT_SUCCESS(PhDeviceIoControlFile(
        DosDeviceHandle,
        FSCTL_GET_REFS_VOLUME_DATA, // FSCTL_QUERY_REFS_VOLUME_COUNTER_INFO
        NULL,
        0,
        &result,
        sizeof(result),
        NULL
        )))
    {
        *VolumeInfo = result;
        return TRUE;
    }

    return FALSE;
}

NTSTATUS DiskDriveQueryVolumeInformation(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PFILE_FS_VOLUME_INFORMATION* VolumeInfo
    )
{
    NTSTATUS status;
    ULONG bufferLength;
    IO_STATUS_BLOCK isb;
    PFILE_FS_VOLUME_INFORMATION buffer;

    bufferLength = sizeof(FILE_FS_VOLUME_INFORMATION);
    buffer = PhAllocate(bufferLength);
    memset(buffer, 0, bufferLength);

    status = NtQueryVolumeInformationFile(
        DosDeviceHandle,
        &isb,
        buffer,
        bufferLength,
        FileFsVolumeInformation
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        bufferLength = sizeof(FILE_FS_VOLUME_INFORMATION) + buffer->VolumeLabelLength;
        buffer = PhReAllocate(buffer, bufferLength);
        memset(buffer, 0, bufferLength);

        status = NtQueryVolumeInformationFile(
            DosDeviceHandle,
            &isb,
            buffer,
            bufferLength,
            FileFsVolumeInformation
            );
    }

    if (NT_SUCCESS(status))
    {
        *VolumeInfo = buffer;
        return status;
    }

    PhFree(buffer);
    return status;
}

NTSTATUS DiskDriveQueryVolumeAttributes(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PFILE_FS_ATTRIBUTE_INFORMATION* AttributeInfo
    )
{
    NTSTATUS status;
    ULONG bufferLength;
    IO_STATUS_BLOCK isb;
    PFILE_FS_ATTRIBUTE_INFORMATION buffer;

    bufferLength = sizeof(FILE_FS_ATTRIBUTE_INFORMATION);
    buffer = PhAllocate(bufferLength);
    memset(buffer, 0, bufferLength);

    status = NtQueryVolumeInformationFile(
        DosDeviceHandle,
        &isb,
        buffer,
        bufferLength,
        FileFsAttributeInformation
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        bufferLength = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + buffer->FileSystemNameLength;
        buffer = PhReAllocate(buffer, bufferLength);
        memset(buffer, 0, bufferLength);

        status = NtQueryVolumeInformationFile(
            DosDeviceHandle,
            &isb,
            buffer,
            bufferLength,
            FileFsAttributeInformation
            );
    }

    if (NT_SUCCESS(status))
    {
        *AttributeInfo = buffer;
        return status;
    }

    PhFree(buffer);
    return status;
}

PWSTR SmartAttributeGetText(
    _In_ SMART_ATTRIBUTE_ID AttributeId
    )
{
    switch (AttributeId)
    {
    case SMART_ATTRIBUTE_ID_READ_ERROR_RATE: // Critical
        return L"Raw Read Error Rate";
    case SMART_ATTRIBUTE_ID_THROUGHPUT_PERFORMANCE:
        return L"Throughput Performance";
    case SMART_ATTRIBUTE_ID_SPIN_UP_TIME:
        return L"Spin Up Time";
    case SMART_ATTRIBUTE_ID_START_STOP_COUNT:
        return L"Start/Stop Count";
    case SMART_ATTRIBUTE_ID_REALLOCATED_SECTORS_COUNT: // Critical
        return L"Reallocated Sectors Count";
    case SMART_ATTRIBUTE_ID_READ_CHANNEL_MARGIN:
        return L"Read Channel Margin";
    case SMART_ATTRIBUTE_ID_SEEK_ERROR_RATE:
        return L"Seek Error Rate";
    case SMART_ATTRIBUTE_ID_SEEK_TIME_PERFORMANCE:
        return L"Seek Time Performance";
    case SMART_ATTRIBUTE_ID_POWER_ON_HOURS:
        return L"Power-On Hours";
    case SMART_ATTRIBUTE_ID_SPIN_RETRY_COUNT:
        return L"Spin Retry Count";
    case SMART_ATTRIBUTE_ID_CALIBRATION_RETRY_COUNT:
        return L"Recalibration Retries";
    case SMART_ATTRIBUTE_ID_POWER_CYCLE_COUNT:
        return L"Device Power Cycle Count";
    case SMART_ATTRIBUTE_ID_SOFT_READ_ERROR_RATE:
        return L"Soft Read Error Rate";
    case SMART_ATTRIBUTE_ID_SATA_DOWNSHIFT_ERROR_COUNT:
        return L"Sata Downshift Error Count";
    case SMART_ATTRIBUTE_ID_END_TO_END_ERROR:
        return L"End To End Error";
    case SMART_ATTRIBUTE_ID_HEAD_STABILITY:
        return L"Head Stability";
    case SMART_ATTRIBUTE_ID_INDUCED_OP_VIBRATION_DETECTION:
        return L"Induced Op Vibration Detection";
    case SMART_ATTRIBUTE_ID_REPORTED_UNCORRECTABLE_ERRORS:
        return L"Reported Uncorrectable Errors";
    case SMART_ATTRIBUTE_ID_COMMAND_TIMEOUT:
        return L"Command Timeout";
    case SMART_ATTRIBUTE_ID_HIGH_FLY_WRITES:
        return L"High Fly Writes";
    case SMART_ATTRIBUTE_ID_TEMPERATURE_DIFFERENCE_FROM_100:
        return L"Airflow Temperature";
    case SMART_ATTRIBUTE_ID_GSENSE_ERROR_RATE:
        return L"GSense Error Rate";
    case SMART_ATTRIBUTE_ID_POWER_OFF_RETRACT_COUNT:
        return L"Power-off Retract Count";
    case SMART_ATTRIBUTE_ID_LOAD_CYCLE_COUNT:
        return L"Load/Unload Cycle Count";
    case SMART_ATTRIBUTE_ID_TEMPERATURE:
        return L"Temperature";
    case SMART_ATTRIBUTE_ID_HARDWARE_ECC_RECOVERED:
        return L"Hardware ECC Recovered";
    case SMART_ATTRIBUTE_ID_REALLOCATION_EVENT_COUNT: // Critical
        return L"Reallocation Event Count";
    case SMART_ATTRIBUTE_ID_CURRENT_PENDING_SECTOR_COUNT: // Critical
        return L"Current Pending Sector Count";
    case SMART_ATTRIBUTE_ID_UNCORRECTABLE_SECTOR_COUNT: // Critical
        return L"Uncorrectable Sector Count";
    case SMART_ATTRIBUTE_ID_ULTRADMA_CRC_ERROR_COUNT:
        return L"UltraDMA CRC Error Count";
    case SMART_ATTRIBUTE_ID_MULTI_ZONE_ERROR_RATE:
        return L"Write Error Rate";
    case SMART_ATTRIBUTE_ID_OFFTRACK_SOFT_READ_ERROR_RATE:
        return L"Soft read error rate";
    case SMART_ATTRIBUTE_ID_DATA_ADDRESS_MARK_ERRORS:
        return L"Data Address Mark errors";
    case SMART_ATTRIBUTE_ID_RUN_OUT_CANCEL:
        return L"Run out cancel";
    case SMART_ATTRIBUTE_ID_SOFT_ECC_CORRECTION:
        return L"Soft ECC correction";
    case SMART_ATTRIBUTE_ID_THERMAL_ASPERITY_RATE_TAR:
        return L"Thermal asperity rate (TAR)";
    case SMART_ATTRIBUTE_ID_FLYING_HEIGHT:
        return L"Flying height";
    case SMART_ATTRIBUTE_ID_SPIN_HIGH_CURRENT:
        return L"Spin high current";
    case SMART_ATTRIBUTE_ID_SPIN_BUZZ:
        return L"Spin buzz";
    case SMART_ATTRIBUTE_ID_OFFLINE_SEEK_PERFORMANCE:
        return L"Offline seek performance";
    case SMART_ATTRIBUTE_ID_VIBRATION_DURING_WRITE:
        return L"Vibration During Write";
    case SMART_ATTRIBUTE_ID_SHOCK_DURING_WRITE:
        return L"Shock During Write";
    case SMART_ATTRIBUTE_ID_DISK_SHIFT: // Critical
        return L"Disk Shift";
    case SMART_ATTRIBUTE_ID_GSENSE_ERROR_RATE_ALT:
        return L"G-Sense Error Rate (Alt)";
    case SMART_ATTRIBUTE_ID_LOADED_HOURS:
        return L"Loaded Hours";
    case SMART_ATTRIBUTE_ID_LOAD_UNLOAD_RETRY_COUNT:
        return L"Load/Unload Retry Count";
    case SMART_ATTRIBUTE_ID_LOAD_FRICTION:
        return L"Load Friction";
    case SMART_ATTRIBUTE_ID_LOAD_UNLOAD_CYCLE_COUNT:
        return L"Load Unload Cycle Count";
    case SMART_ATTRIBUTE_ID_LOAD_IN_TIME:
        return L"Load-in Time";
    case SMART_ATTRIBUTE_ID_TORQUE_AMPLIFICATION_COUNT:
        return L"Torque Amplification Count";
    case SMART_ATTRIBUTE_ID_POWER_OFF_RETTRACT_CYCLE:
        return L"Power-Off Retract Count";
    case SMART_ATTRIBUTE_ID_GMR_HEAD_AMPLITUDE:
        return L"GMR Head Amplitude";
    case SMART_ATTRIBUTE_ID_DRIVE_TEMPERATURE:
        return L"Temperature";
    case SMART_ATTRIBUTE_ID_HEAD_FLYING_HOURS:
        return L"Head Flying Hours";
    case SMART_ATTRIBUTE_ID_TOTAL_LBA_WRITTEN:
        return L"Total LBAs Written";
    case SMART_ATTRIBUTE_ID_TOTAL_LBA_READ:
        return L"Total LBAs Read";
    case SMART_ATTRIBUTE_ID_READ_ERROR_RETY_RATE:
        return L"Read Error Retry Rate";
    case SMART_ATTRIBUTE_ID_FREE_FALL_PROTECTION:
        return L"Free Fall Protection";
    case SMART_ATTRIBUTE_ID_SSD_PROGRAM_FAIL_COUNT:
        return L"SSD Program Fail Count";
    case SMART_ATTRIBUTE_ID_SSD_ERASE_FAIL_COUNT:
        return L"SSD Erase Fail Count";
    case SMART_ATTRIBUTE_ID_SSD_WEAR_LEVELING_COUNT:
        return L"SSD Wear Leveling Count";
    case SMART_ATTRIBUTE_ID_UNEXPECTED_POWER_LOSS:
        return L"Unexpected power loss count";
    case SMART_ATTRIBUTE_ID_WEAR_RANGE_DELTA:
        return L"Wear Range Delta";
    case SMART_ATTRIBUTE_ID_SSD_PROGRAM_FAIL_COUNT_TOTAL:
        return L"Program Fail Count Total";
    case SMART_ATTRIBUTE_ID_ERASE_FAIL_COUNT:
        return L"Erase Fail Count";
    case SMART_ATTRIBUTE_ID_SSD_MEDIA_WEAROUT_HOURS:
        return L"Media Wearout Indicator";
    case SMART_ATTRIBUTE_ID_SSD_ERASE_COUNT:
        return L"Erase count";
    case SMART_ATTRIBUTE_ID_MIN_SPARES_REMAINING:
        return L"Minimum Spares Remaining";
    case SMART_ATTRIBUTE_ID_NEWLY_ADDED_BAD_FLASH_BLOCK:
        return L"Newly Added Bad Flash Block";
    }

    return L"BUG BUG BUG";
}

NTSTATUS DiskDriveQueryUniqueId(
    _In_ HANDLE DeviceHandle,
    _Out_ PPH_STRING* UniqueId,
    _Out_ PPH_STRING* PartitionId
    )
{
    NTSTATUS status;
    ULONG bufferLength;
    PDRIVE_LAYOUT_INFORMATION_EX buffer;

    bufferLength = sizeof(DRIVE_LAYOUT_INFORMATION_EX[10]);
    buffer = PhAllocate(bufferLength);
    memset(buffer, 0, bufferLength);

    status = PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
        NULL,
        0,
        buffer,
        bufferLength,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PH_STRING_BUILDER stringBuilder;

        if (buffer->PartitionStyle != PARTITION_STYLE_GPT)
            *UniqueId = PhReferenceEmptyString();
        else
            *UniqueId = PhFormatGuid(&buffer->Gpt.DiskId); // DISKPART> UNIQUEID DISK

        PhInitializeStringBuilder(&stringBuilder, 0x100);

        for (ULONG i = 0; i < buffer->PartitionCount; i++)
        {
            PARTITION_INFORMATION_EX entry = buffer->PartitionEntry[i];
            PPH_STRING id;

            if (entry.PartitionStyle == PARTITION_STYLE_GPT)
            {
                if (id = PhFormatGuid(&entry.Gpt.PartitionId))
                {
                    PhAppendStringBuilder(&stringBuilder, &id->sr);
                    PhAppendFormatStringBuilder(&stringBuilder, L", ");
                }
            }
        }

        if (PhEndsWithStringRef2(&stringBuilder.String->sr, L", ", FALSE))
            PhRemoveEndStringBuilder(&stringBuilder, 2);

        *PartitionId = PhFinalStringBuilderString(&stringBuilder);
    }

    PhFree(buffer);

    return status;
}

NTSTATUS DiskDriveQueryPartitionInfo(
    _In_ HANDLE DeviceHandle,
    _Out_ PARTITION_INFORMATION_EX* PartitionInfo
    )
{
    NTSTATUS status;

    status = PhDeviceIoControlFile(
        DeviceHandle,
        IOCTL_DISK_GET_PARTITION_INFO_EX,
        NULL,
        0,
        PartitionInfo,
        sizeof(PARTITION_INFORMATION_EX),
        NULL
        );

    return status;
}

//BOOLEAN DiskDriveQueryAdapterInformation(
//    _In_ HANDLE DeviceHandle
//    )
//{
//    ULONG bufferLength;
//    STORAGE_PROPERTY_QUERY query;
//    PSTORAGE_ADAPTER_DESCRIPTOR buffer;
//
//    query.QueryType = PropertyStandardQuery;
//    query.PropertyId = StorageAdapterProperty;
//
//    bufferLength = sizeof(STORAGE_ADAPTER_DESCRIPTOR);
//    buffer = PhAllocate(bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    if (!NT_SUCCESS(PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_STORAGE_QUERY_PROPERTY,
//        &query,
//        sizeof(query),
//        buffer,
//        bufferLength,
//        NULL
//        )))
//    {
//        PhFree(buffer);
//        return FALSE;
//    }
//
//    bufferLength = buffer->Size;
//    buffer = PhReAllocate(buffer, bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    if (!NT_SUCCESS(PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_STORAGE_QUERY_PROPERTY,
//        &query,
//        sizeof(query),
//        buffer,
//        bufferLength,
//        NULL
//        )))
//    {
//        PhFree(buffer);
//        return FALSE;
//    }
//
//    PSTORAGE_ADAPTER_DESCRIPTOR storageDescriptor = (PSTORAGE_ADAPTER_DESCRIPTOR)buffer;
//
//    if (buffer)
//    {
//        PhFree(buffer);
//    }
//
//    return TRUE;
//}
//
//BOOLEAN DiskDriveQueryCache(
//    _In_ HANDLE DeviceHandle
//    )
//{
//    ULONG bufferLength;
//    STORAGE_PROPERTY_QUERY query;
//    PSTORAGE_WRITE_CACHE_PROPERTY buffer;
//
//    query.QueryType = PropertyStandardQuery;
//    query.PropertyId = StorageDeviceWriteCacheProperty;
//
//    bufferLength = sizeof(STORAGE_WRITE_CACHE_PROPERTY);
//    buffer = PhAllocate(bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    if (!NT_SUCCESS(PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_STORAGE_QUERY_PROPERTY,
//        &query,
//        sizeof(query),
//        buffer,
//        bufferLength,
//        NULL
//        )))
//    {
//        PhFree(buffer);
//        return FALSE;
//    }
//
//    bufferLength = buffer->Size;
//    buffer = PhReAllocate(buffer, bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    if (!NT_SUCCESS(PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_STORAGE_QUERY_PROPERTY,
//        &query,
//        sizeof(query),
//        buffer,
//        bufferLength,
//        NULL
//        )))
//    {
//        PhFree(buffer);
//        return FALSE;
//    }
//
//    PSTORAGE_WRITE_CACHE_PROPERTY storageDescriptor = (PSTORAGE_WRITE_CACHE_PROPERTY)buffer;
//
//    if (buffer)
//    {
//        PhFree(buffer);
//    }
//
//    return TRUE;
//}
//
//BOOLEAN DiskDriveQueryPower(
//    _In_ HANDLE DeviceHandle
//    )
//{
//    ULONG bufferLength;
//    STORAGE_PROPERTY_QUERY query;
//    PDEVICE_POWER_DESCRIPTOR buffer;
//
//    query.QueryType = PropertyStandardQuery;
//    query.PropertyId = StorageDevicePowerProperty;
//
//    bufferLength = sizeof(DEVICE_POWER_DESCRIPTOR);
//    buffer = PhAllocate(bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    if (!NT_SUCCESS(PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_STORAGE_QUERY_PROPERTY,
//        &query,
//        sizeof(query),
//        buffer,
//        bufferLength,
//        NULL
//        )))
//    {
//        PhFree(buffer);
//        return FALSE;
//    }
//
//    bufferLength = buffer->Size;
//    buffer = PhReAllocate(buffer, bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    if (!NT_SUCCESS(PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_STORAGE_QUERY_PROPERTY,
//        &query,
//        sizeof(query),
//        buffer,
//        bufferLength,
//        NULL
//        )))
//    {
//        PhFree(buffer);
//        return FALSE;
//    }
//
//    PDEVICE_POWER_DESCRIPTOR storageDescriptor = (PDEVICE_POWER_DESCRIPTOR)buffer;
//
//    if (buffer)
//    {
//        PhFree(buffer);
//    }
//
//    return TRUE;
//}
//
//BOOLEAN DiskDriveQueryTemperature(
//    _In_ HANDLE DeviceHandle
//    )
//{
//    ULONG bufferLength;
//    STORAGE_PROPERTY_QUERY query;
//    PSTORAGE_TEMPERATURE_DATA_DESCRIPTOR buffer;
//
//    query.QueryType = PropertyStandardQuery;
//    query.PropertyId = StorageAdapterTemperatureProperty; // StorageDeviceTemperatureProperty
//
//    bufferLength = sizeof(STORAGE_TEMPERATURE_DATA_DESCRIPTOR);
//    buffer = PhAllocate(bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    if (!NT_SUCCESS(PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_STORAGE_QUERY_PROPERTY,
//        &query,
//        sizeof(query),
//        buffer,
//        bufferLength,
//        NULL
//        )))
//    {
//        PhFree(buffer);
//        return FALSE;
//    }
//
//    bufferLength = buffer->Size;
//    buffer = PhReAllocate(buffer, bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    if (!NT_SUCCESS(PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_STORAGE_QUERY_PROPERTY,
//        &query,
//        sizeof(query),
//        buffer,
//        bufferLength,
//        NULL
//        )))
//    {
//        PhFree(buffer);
//        return FALSE;
//    }
//
//    PSTORAGE_TEMPERATURE_DATA_DESCRIPTOR storageDescriptor = (PSTORAGE_TEMPERATURE_DATA_DESCRIPTOR)buffer;
//
//    if (buffer)
//    {
//        PhFree(buffer);
//    }
//
//    return TRUE;
//}
//
//BOOLEAN DiskDriveQueryTxfsVolumeInfo(
//    _In_ HANDLE DosDeviceHandle
//    )
//{
//    ULONG bufferLength;
//    PTXFS_LIST_TRANSACTIONS buffer;
//
//    bufferLength = sizeof(TXFS_LIST_TRANSACTIONS);
//    buffer = PhAllocate(bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    //HANDLE deviceHandle = CreateFile(
//    //    L"C:\\",
//    //    GENERIC_READ,
//    //    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
//    //    NULL,
//    //    OPEN_EXISTING,
//    //    FILE_FLAG_BACKUP_SEMANTICS,
//    //    NULL
//    //    );
//
//    PhDeviceIoControlFile(
//        DosDeviceHandle,
//        FSCTL_TXFS_LIST_TRANSACTIONS,
//        NULL,
//        0,
//        buffer,
//        bufferLength,
//        NULL
//        );
//
//    bufferLength = (ULONG)buffer->BufferSizeRequired;
//    buffer = PhReAllocate(buffer, bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    PhDeviceIoControlFile(
//        DosDeviceHandle,
//        FSCTL_TXFS_LIST_TRANSACTIONS,
//        NULL,
//        0,
//        buffer,
//        bufferLength,
//        NULL
//        );
//
//    for (ULONG i = 0; i < buffer->NumberOfTransactions; i++)
//    {
//        //PTXFS_LIST_TRANSACTIONS_ENTRY entry = (PTXFS_LIST_TRANSACTIONS_ENTRY)(buffer + i * sizeof(TXFS_LIST_TRANSACTIONS));
//        //PPH_STRING txGuid = PhFormatGuid(&entry->TransactionId);
//        //entry->TransactionState;
//        //Resource Manager Identifier :     17DC1CDD-9C6C-11E5-BBC2-F5C37BC15998
//    }
//
//    return FALSE;
//}
//
//BOOLEAN DiskDriveQueryBootSectorFsCount(
//    _In_ HANDLE DosDeviceHandle
//    )
//{
//    BOOT_AREA_INFO result;
//
//    memset(&result, 0, sizeof(BOOT_AREA_INFO));
//
//    if (!NT_SUCCESS(PhDeviceIoControlFile(
//        DosDeviceHandle,
//        FSCTL_GET_BOOT_AREA_INFO,
//        NULL,
//        0,
//        &result,
//        sizeof(result),
//        NULL
//        )))
//    {
//        return FALSE;
//    }
//
//    for (ULONG i = 0; i < result.BootSectorCount; i++)
//    {
//        // https://msdn.microsoft.com/en-us/library/windows/desktop/dd442654.aspx
//        typedef struct _FILE_SYSTEM_RECOGNITION_STRUCTURE
//        {
//            UCHAR Jmp[3];
//            UCHAR FsName[8];
//            UCHAR MustBeZero[5];
//            ULONG Identifier;
//            USHORT Length;
//            USHORT Checksum;
//        } FILE_SYSTEM_RECOGNITION_STRUCTURE;
//
//#include <pshpack1.h>
//        typedef struct _BOOT_SECTOR
//        {
//            UCHAR Jmp[3];
//            UCHAR Format[8];
//            USHORT BytesPerSector;
//            UCHAR SectorsPerCluster;
//            USHORT ReservedSectorCount;
//            UCHAR Mbz1;
//            USHORT Mbz2;
//            USHORT Reserved1;
//            UCHAR MediaType;
//            USHORT Mbz3;
//            USHORT SectorsPerTrack;
//            USHORT NumberOfHeads;
//            ULONG PartitionOffset;
//            ULONG Reserved2[2];
//            ULONGLONG TotalSectors;
//            ULONGLONG MftStartLcn;
//            ULONGLONG Mft2StartLcn;
//            ULONG ClustersPerFileRecord;
//            ULONG ClustersPerIndexBlock;
//            ULONGLONG VolumeSerialNumber;
//            UCHAR Code[0x1AE];  // 430
//            USHORT BootSignature;
//        } BOOT_SECTOR, *PBOOT_SECTOR;
//#include <poppack.h>
//
//        BOOT_SECTOR sector;
//        IO_STATUS_BLOCK isb;
//
//        memset(&sector, 0, sizeof(BOOT_SECTOR));
//
//        // There are 2 boot sectors on NTFS partitions, we can't access the second one.
//        // To read or write to the last few sectors of a volume (where the second boot sector is located),
//        // you must call FSCTL_ALLOW_EXTENDED_DASD_IO, which instructs the file system to not perform any boundary checks,
//        // which doesn't work for some reason.
//        if (!NT_SUCCESS(NtReadFile(
//            DosDeviceHandle,
//            NULL,
//            NULL,
//            NULL,
//            &isb,
//            &sector,
//            sizeof(BOOT_SECTOR),
//            &result.BootSectors[i].Offset,
//            NULL
//            )))
//        {
//
//        }
//    }
//
//    return FALSE;
//}
//
//_Success_(return)
//BOOLEAN DiskDriveQueryVolumeDirty(
//    _In_ HANDLE DosDeviceHandle,
//    _Out_ PBOOLEAN IsDirty
//    )
//{
//    ULONG result;
//
//    memset(&result, 0, sizeof(ULONG));
//
//    if (NT_SUCCESS(PhDeviceIoControlFile(
//        DosDeviceHandle,
//        FSCTL_IS_VOLUME_DIRTY,
//        NULL,
//        0,
//        &result,
//        sizeof(result),
//        NULL
//        )))
//    {
//        if (result & VOLUME_IS_DIRTY)
//        {
//            *IsDirty = TRUE;
//        }
//        else
//        {
//            *IsDirty = FALSE;
//        }
//
//        return TRUE;
//    }
//
//    return FALSE;
//}
//
//_Success_(return)
//BOOLEAN DiskDriveQueryVolumeFreeSpace(
//    _In_ HANDLE DosDeviceHandle,
//    _Out_ ULONG64* TotalLength,
//    _Out_ ULONG64* FreeLength
//    )
//{
//    IO_STATUS_BLOCK isb;
//    FILE_FS_FULL_SIZE_INFORMATION result;
//
//    memset(&result, 0, sizeof(FILE_FS_FULL_SIZE_INFORMATION));
//
//    if (NT_SUCCESS(NtQueryVolumeInformationFile(
//        DosDeviceHandle,
//        &isb,
//        &result,
//        sizeof(FILE_FS_FULL_SIZE_INFORMATION),
//        FileFsFullSizeInformation
//        )))
//    {
//        *TotalLength = result.TotalAllocationUnits.QuadPart * result.SectorsPerAllocationUnit * result.BytesPerSector;
//        *FreeLength = result.ActualAvailableAllocationUnits.QuadPart * result.SectorsPerAllocationUnit * result.BytesPerSector;
//        return TRUE;
//    }
//
//    return FALSE;
//}
//
//BOOLEAN DiskDriveFlushCache(
//    _In_ HANDLE DosDeviceHandle
//    )
//{
//    IO_STATUS_BLOCK isb;
//
//    //  NtFlushBuffersFile
//    //
//    //  If a volume handle is specified:
//    //      - Write all modified data for all files on the volume from the Windows
//    //        in-memory cache.
//    //      - Commit all pending metadata changes for all files on the volume from
//    //        the Windows in-memory cache.
//    //      - Send a SYNC command to the underlying storage device to commit all
//    //        written data in the devices cache to persistent storage.
//    //
//
//    if (NT_SUCCESS(NtFlushBuffersFile(DosDeviceHandle, &isb)))
//        return TRUE;
//
//    return FALSE;
//}
//
//NTSTATUS DiskDriveQueryCacheInformation(
//    _In_ HANDLE DeviceHandle
//    )
//{
//    NTSTATUS status;
//    DISK_CACHE_INFORMATION result;
//
//    memset(&result, 0, sizeof(DISK_CACHE_INFORMATION));
//
//    status = PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_DISK_GET_CACHE_INFORMATION, // requires admin
//        NULL,
//        0,
//        &result,
//        sizeof(result),
//        NULL
//        );
//
//    return status;
//}
//
//NTSTATUS DiskDriveQueryAttributes(
//    _In_ HANDLE DeviceHandle
//    )
//{
//    NTSTATUS status;
//    GET_DISK_ATTRIBUTES result;
//
//    memset(&result, 0, sizeof(GET_DISK_ATTRIBUTES));
//
//    status = PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_DISK_GET_DISK_ATTRIBUTES,
//        NULL,
//        0,
//        &result,
//        sizeof(result),
//        NULL
//        );
//
//    return status;
//}
//
//BOOLEAN DiskDriveQueryBcProperties(
//    _In_ HANDLE DeviceHandle
//    )
//{
//    ULONG bufferLength;
//    PSTORAGE_GET_BC_PROPERTIES_OUTPUT buffer;
//
//    bufferLength = sizeof(STORAGE_GET_BC_PROPERTIES_OUTPUT);
//    buffer = PhAllocate(bufferLength);
//    memset(buffer, 0, bufferLength);
//
//    if (!NT_SUCCESS(PhDeviceIoControlFile(
//        DeviceHandle,
//        IOCTL_STORAGE_GET_BC_PROPERTIES,
//        NULL,
//        0,
//        buffer,
//        bufferLength,
//        NULL
//        )))
//    {
//        PhFree(buffer);
//        return FALSE;
//    }
//
//    PSTORAGE_GET_BC_PROPERTIES_OUTPUT storageDescriptor = (PSTORAGE_GET_BC_PROPERTIES_OUTPUT)buffer;
//
//    if (buffer)
//    {
//        PhFree(buffer);
//    }
//
//    return TRUE;
//}

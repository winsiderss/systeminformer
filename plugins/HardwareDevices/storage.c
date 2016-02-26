/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2015-2016 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "devices.h"
#include <ntdddisk.h>

NTSTATUS DiskDriveCreateHandle(
    _Out_ PHANDLE DeviceHandle,
    _In_ PPH_STRING DevicePath
    )
{
    // Some examples of paths that can be used to open the disk device for statistics:
    // \\.\PhysicalDrive1
    // \\.\X:
    // \\.\Volume{a978c827-cf64-44b4-b09a-57a55ef7f49f}
    // IOCTL_MOUNTMGR_QUERY_POINTS (used by FindFirstVolume and FindFirstVolumeMountPoint)
    // HKEY_LOCAL_MACHINE\\SYSTEM\\MountedDevices (contains the DosDevice and path used by the SetupAPI with DetailData->DevicePath)
    // Other methods??

    return PhCreateFileWin32(
        DeviceHandle,
        DevicePath->Buffer,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );
}

ULONG DiskDriveQueryDeviceMap(VOID)
{
#ifndef _WIN64
    PROCESS_DEVICEMAP_INFORMATION deviceMapInfo;
#else 
    PROCESS_DEVICEMAP_INFORMATION_EX deviceMapInfo;
#endif

    memset(&deviceMapInfo, 0, sizeof(deviceMapInfo));

    if (NT_SUCCESS(NtQueryInformationProcess(
        NtCurrentProcess(),
        ProcessDeviceMap,
        &deviceMapInfo,
        sizeof(deviceMapInfo),
        NULL
        )))
    {
        return deviceMapInfo.Query.DriveMap;
    }
    else
    {
        return GetLogicalDrives();
    }
}

PPH_STRING DiskDriveQueryDosMountPoints(
    _In_ ULONG DeviceNumber
    )
{
    ULONG driveMask;
    PH_STRING_BUILDER stringBuilder;
    WCHAR devicePath[] = L"\\\\.\\?:";

    PhInitializeStringBuilder(&stringBuilder, MAX_PATH);

    driveMask = DiskDriveQueryDeviceMap();

    // NOTE: This isn't the best way of doing this but it works (It's also what Task Manager does).
    for (INT i = 0; i < 0x1A; i++)
    {
        if (driveMask & (0x1 << i))
        {
            HANDLE deviceHandle;

            devicePath[4] = 'A' + i;

            if (NT_SUCCESS(PhCreateFileWin32(
                &deviceHandle,
                devicePath,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                )))
            {
                ULONG deviceNumber = ULONG_MAX; // Note: Do not initialize to zero.

                if (NT_SUCCESS(DiskDriveQueryDeviceTypeAndNumber(
                    deviceHandle,
                    &deviceNumber,
                    NULL
                    )));
                {
                    if (deviceNumber == DeviceNumber)
                    {
                        PhAppendFormatStringBuilder(&stringBuilder, L"%c: ", devicePath[4]);
                    }
                }

                NtClose(deviceHandle);
            }
        }
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

BOOLEAN DiskDriveQueryDeviceInformation(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PPH_STRING* DiskVendor,
    _Out_opt_ PPH_STRING* DiskModel,
    _Out_opt_ PPH_STRING* DiskRevision,
    _Out_opt_ PPH_STRING* DiskSerial
    )
{
    ULONG bufferLength;
    IO_STATUS_BLOCK isb;
    STORAGE_PROPERTY_QUERY query;
    PSTORAGE_DESCRIPTOR_HEADER buffer;

    query.QueryType = PropertyStandardQuery;
    query.PropertyId = StorageDeviceProperty;

    bufferLength = sizeof(STORAGE_DESCRIPTOR_HEADER);
    buffer = PhAllocate(bufferLength);
    memset(buffer, 0, bufferLength);

    if (!NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_STORAGE_QUERY_PROPERTY, // https://msdn.microsoft.com/en-us/library/ff800830.aspx
        &query,
        sizeof(query),
        buffer,
        bufferLength
        )))
    {
        PhFree(buffer);
        return FALSE;
    }

    bufferLength = buffer->Size;
    buffer = PhReAllocate(buffer, bufferLength);
    memset(buffer, 0, bufferLength);

    if (!NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &query,
        sizeof(query),
        buffer,
        bufferLength
        )))
    {
        PhFree(buffer);
        return FALSE;
    }

    PSTORAGE_DEVICE_DESCRIPTOR storageDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)buffer;

    if (DiskVendor && storageDescriptor->VendorIdOffset != 0)
    {
        PPH_STRING diskVendor;

        diskVendor = PH_AUTO(PhConvertMultiByteToUtf16((PBYTE)storageDescriptor + storageDescriptor->VendorIdOffset));

        *DiskVendor = TrimString(diskVendor);
    }

    if (DiskModel && storageDescriptor->ProductIdOffset != 0)
    {
        PPH_STRING diskModel;

        diskModel = PH_AUTO(PhConvertMultiByteToUtf16((PBYTE)storageDescriptor + storageDescriptor->ProductIdOffset));

        *DiskModel = TrimString(diskModel);
    }

    if (DiskRevision && storageDescriptor->ProductRevisionOffset != 0)
    {
        PPH_STRING diskRevision;

        diskRevision = PH_AUTO(PhConvertMultiByteToUtf16((PBYTE)storageDescriptor + storageDescriptor->ProductRevisionOffset));

        *DiskRevision = TrimString(diskRevision);
    }

    if (DiskSerial && storageDescriptor->SerialNumberOffset != 0)
    {
        PPH_STRING diskSerial;

        diskSerial = PH_AUTO(PhConvertMultiByteToUtf16((PBYTE)storageDescriptor + storageDescriptor->SerialNumberOffset));

        *DiskSerial = TrimString(diskSerial);
    }

    if (buffer)
    {
        PhFree(buffer);
    }

    return TRUE;
}

NTSTATUS DiskDriveQueryDeviceTypeAndNumber(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PULONG DeviceNumber,
    _Out_opt_ DEVICE_TYPE* DeviceType
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    STORAGE_DEVICE_NUMBER result;

    memset(&result, 0, sizeof(STORAGE_DEVICE_NUMBER));

    status = NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_STORAGE_GET_DEVICE_NUMBER, // https://msdn.microsoft.com/en-us/library/bb968800.aspx
        NULL,
        0,
        &result,
        sizeof(result)
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
    IO_STATUS_BLOCK isb;
    DISK_PERFORMANCE result;

    memset(&result, 0, sizeof(DISK_PERFORMANCE));

    // TODO: IOCTL_DISK_GET_PERFORMANCE_INFO from ntdddisk.h
    status = NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_DISK_PERFORMANCE, // https://msdn.microsoft.com/en-us/library/aa365183.aspx
        NULL,
        0,
        &result,
        sizeof(result)
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
    IO_STATUS_BLOCK isb;
    DISK_GEOMETRY result;

    memset(&result, 0, sizeof(DISK_GEOMETRY));

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_DISK_GET_DRIVE_GEOMETRY, // https://msdn.microsoft.com/en-us/library/aa365169.aspx
        NULL,
        0,
        &result,
        sizeof(result)
        )))
    {
        // TODO: This doesn't return total capacity like Task Manager.
        return PhFormatSize(result.Cylinders.QuadPart * result.TracksPerCylinder * result.SectorsPerTrack * result.BytesPerSector, -1);
    }

    return PhReferenceEmptyString();
}

BOOLEAN DiskDriveQueryImminentFailure(
    _In_ HANDLE DeviceHandle,
    _Out_ PPH_LIST* DiskSmartAttributes
    )
{
    IO_STATUS_BLOCK isb;
    STORAGE_PREDICT_FAILURE storagePredictFailure;

    memset(&storagePredictFailure, 0, sizeof(STORAGE_PREDICT_FAILURE));
    
    // * IOCTL_STORAGE_PREDICT_FAILURE returns an opaque 512-byte vendor-specific information block, which
    //      in all cases contains SMART attribute information (2 bytes header + 12 bytes each attribute).
    // * This works without admin rights but doesn't support other features like logs and self-tests.
    // * It works for (S)ATA devices but not for USB.

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_STORAGE_PREDICT_FAILURE, // https://msdn.microsoft.com/en-us/library/ff560587.aspx
        NULL,
        0,
        &storagePredictFailure,
        sizeof(storagePredictFailure)
        )))
    {
        PPH_LIST diskAttributeList;
        //USHORT structureVersion = (USHORT)(storagePredictFailure.VendorSpecific[0] * 256 + storagePredictFailure.VendorSpecific[1]);
        //USHORT majorVersion = HIBYTE(structureVersion);
        //USHORT minorVersion = LOBYTE(structureVersion);
        //TODO: include storagePredictFailure.PredictFailure;

        diskAttributeList = PhCreateList(30);

        for (UCHAR i = 0; i < 30; ++i)
        {
            // This could be better?
            PSMART_ATTRIBUTE attribute = (PSMART_ATTRIBUTE)&storagePredictFailure.VendorSpecific[i * sizeof(SMART_ATTRIBUTE) + 2]; //TODO: Parse +2 attribute header

            if (
                attribute->Id != 0x00 && // Attribute values 0x00, 0xFE, 0xFF are invalid (TODO: Are invalid values vendor specific?)
                attribute->Id != 0xFE &&
                attribute->Id != 0xFF
                )
            {
                PSMART_ATTRIBUTES info = PhAllocate(sizeof(SMART_ATTRIBUTES));
                memset(info, 0, sizeof(SMART_ATTRIBUTES));

                info->AttributeId = attribute->Id;
                info->CurrentValue = attribute->CurrentValue;
                info->WorstValue = attribute->WorstValue;
                info->Advisory = (attribute->Flags & 0x1) == 0x0;
                info->FailureImminent = (attribute->Flags & 0x1) == 0x1;
                info->OnlineDataCollection = (attribute->Flags & 0x2) == 0x2;

                PhAddItemList(diskAttributeList, info);
            }
        }

        *DiskSmartAttributes = diskAttributeList;

        return TRUE;
    }

    return FALSE;
}


PWSTR SmartAttributeGetText(
    _In_ SMART_ATTRIBUTE_ID AttributeId
    )
{
    // from https://en.wikipedia.org/wiki/S.M.A.R.T

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
    case SMART_ATTRIBUTE_ID_HEAD_FLYING_HOURS: // Transfer Error Rate (Fujitsu)
        return L"Head Flying Hours";
    case SMART_ATTRIBUTE_ID_TOTAL_LBA_WRITTEN:
        return L"Total LBAs Written";
    case SMART_ATTRIBUTE_ID_TOTAL_LBA_READ:
        return L"Total LBAs Read";
    case SMART_ATTRIBUTE_ID_READ_ERROR_RETY_RATE:
        return L"Read Error Retry Rate";
    case SMART_ATTRIBUTE_ID_FREE_FALL_PROTECTION:
        return L"Free Fall Protection";
    }

    return L"BUG BUG BUG";
}

PWSTR SmartAttributeGetDescription(
    _In_ SMART_ATTRIBUTE_ID AttributeId
    )
{
    // from https://en.wikipedia.org/wiki/S.M.A.R.T
    switch (AttributeId)
    {
    case SMART_ATTRIBUTE_ID_READ_ERROR_RATE:
        return L"Lower raw value is better.\r\nVendor specific raw value. Stores data related to the rate of hardware read errors that occurred when reading data from a disk surface. The raw value has different structure for different vendors and is often not meaningful as a decimal number.";
    case SMART_ATTRIBUTE_ID_THROUGHPUT_PERFORMANCE:
        return L"Higher raw value is better.\r\nOverall (general) throughput performance of a hard disk drive. If the value of this attribute is decreasing there is a high probability that there is a problem with the disk.";
    case SMART_ATTRIBUTE_ID_SPIN_UP_TIME:
        return L"Lower raw value is better.\r\nAverage time of spindle spin up (from zero RPM to fully operational [milliseconds]).";
    case SMART_ATTRIBUTE_ID_START_STOP_COUNT:
        return L"A tally of spindle start/stop cycles.\r\nThe spindle turns on, and hence the count is increased, both when the hard disk is turned on after having before been turned entirely off (disconnected from power source) and when the hard disk returns from having previously been put to sleep mode.";
    case SMART_ATTRIBUTE_ID_REALLOCATED_SECTORS_COUNT:
        return L"Lower raw value is better.\r\nCount of reallocated sectors. When the hard drive finds a read/write/verification error, it marks that sector as \"reallocated\" and transfers data to a special reserved area (spare area). This process is also known as remapping, and reallocated sectors are called \"remaps\". The raw value normally represents a count of the bad sectors that have been found and remapped. Thus, the higher the attribute value, the more sectors the drive has had to reallocate. This allows a drive with bad sectors to continue operation; however, a drive which has had any reallocations at all is significantly more likely to fail in the near future. While primarily used as a metric of the life expectancy of the drive, this number also affects performance. As the count of reallocated sectors increases, the read/write speed tends to become worse because the drive head is forced to seek to the reserved area whenever a remap is accessed. If sequential access speed is critical, the remapped sectors can be manually marked as bad blocks in the file system in order to prevent their use.";
    case SMART_ATTRIBUTE_ID_READ_CHANNEL_MARGIN:
        return L"Margin of a channel while reading data.\r\nThe function of this attribute is not specified.";
    case SMART_ATTRIBUTE_ID_SEEK_ERROR_RATE:
        return L"Vendor specific raw value.\r\nRate of seek errors of the magnetic heads. If there is a partial failure in the mechanical positioning system, then seek errors will arise. Such a failure may be due to numerous factors, such as damage to a servo, or thermal widening of the hard disk. The raw value has different structure for different vendors and is often not meaningful as a decimal number.";
    case SMART_ATTRIBUTE_ID_SEEK_TIME_PERFORMANCE:
        return L"Average performance of seek operations of the magnetic heads.\r\nIf this attribute is decreasing, it is a sign of problems in the mechanical subsystem.";
    case SMART_ATTRIBUTE_ID_POWER_ON_HOURS:
        return L"Count of hours in power-on state.\r\nThe raw value of this attribute shows total count of hours (or minutes, or seconds, depending on manufacturer) in power-on state.\r\nBy default, the total expected lifetime of a hard disk in perfect condition is defined as 5 years(running every day and night on all days).This is equal to 1825 days in 24 / 7 mode or 43800 hours.\r\nOn some pre-2005 drives, this raw value may advance erratically and/or \"wrap around\" (reset to zero periodically).";
    case SMART_ATTRIBUTE_ID_SPIN_RETRY_COUNT:
        return L"Count of retry of spin start attempts.\r\nThis attribute stores a total count of the spin start attempts to reach the fully operational speed (under the condition that the first attempt was unsuccessful). An increase of this attribute value is a sign of problems in the hard disk mechanical subsystem.";
    case SMART_ATTRIBUTE_ID_CALIBRATION_RETRY_COUNT:
        return L"This attribute indicates the count that recalibration was requested (under the condition that the first attempt was unsuccessful). An increase of this attribute value is a sign of problems in the hard disk mechanical subsystem.";
    case SMART_ATTRIBUTE_ID_POWER_CYCLE_COUNT:
        return L"This attribute indicates the count of full hard disk power on/off cycles.";
    case SMART_ATTRIBUTE_ID_SOFT_READ_ERROR_RATE:
        return L"Uncorrected read errors reported to the operating system.";

    //TODO: Include more descriptions..
    }

    return L"";
}
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

NTSTATUS DiskDriveCreateHandle(
    _Out_ PHANDLE DeviceHandle,
    _In_ ULONG DeviceNumber
    )
{
    // \\.\PhysicalDrive1
    // \\.\X:
    // \\.\Volume{a978c827-cf64-44b4-b09a-57a55ef7f49f}
    // SetupAPI with GUID_DEVINTERFACE_DISK and DetailData->DevicePath

    return PhCreateFileWin32(
        DeviceHandle,
        PhaFormatString(L"\\\\.\\PhysicalDrive%lu", DeviceNumber)->Buffer,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );
}

BOOLEAN DiskDriveQueryDeviceInformation(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PPH_STRING* DiskVendor,
    _Out_opt_ PPH_STRING* DiskModel,
    _Out_opt_ PPH_STRING* DiskRevision,
    _Out_opt_ PPH_STRING* DiskSerial
    )
{
    static PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" ");

    ULONG bufferLength;
    IO_STATUS_BLOCK isb;
    STORAGE_PROPERTY_QUERY query;
    PSTORAGE_DESCRIPTOR_HEADER buffer = NULL;

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

        diskVendor = PhConvertMultiByteToUtf16((PBYTE)storageDescriptor + storageDescriptor->VendorIdOffset);
        PhTrimStringRef(&diskVendor->sr, &whitespace, 0);

        *DiskVendor = diskVendor;
    }

    if (DiskModel && storageDescriptor->ProductIdOffset != 0)
    {
        PPH_STRING diskModel;

        diskModel = PhConvertMultiByteToUtf16((PBYTE)storageDescriptor + storageDescriptor->ProductIdOffset);
        PhTrimStringRef(&diskModel->sr, &whitespace, 0);

        *DiskModel = diskModel;
    }

    if (DiskRevision && storageDescriptor->ProductRevisionOffset != 0)
    {
        PPH_STRING diskRevision;

        diskRevision = PhConvertMultiByteToUtf16((PBYTE)storageDescriptor + storageDescriptor->ProductRevisionOffset);
        PhTrimStringRef(&diskRevision->sr, &whitespace, 0);

        *DiskRevision = diskRevision;
    }

    if (DiskSerial && storageDescriptor->SerialNumberOffset != 0)
    {
        PPH_STRING diskSerial;

        diskSerial = PhConvertMultiByteToUtf16((PBYTE)storageDescriptor + storageDescriptor->SerialNumberOffset);
        PhTrimStringRef(&diskSerial->sr, &whitespace, 0);

        *DiskSerial = diskSerial;
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

    if (DeviceType)
    {
        *DeviceType = result.DeviceType;
    }

    if (DeviceNumber)
    {
        *DeviceNumber = result.DeviceNumber;
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

    *Info = result;

    return status;
}

NTSTATUS DiskDriveDisableStatistics(
    _In_ HANDLE DeviceHandle
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    status = NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_DISK_PERFORMANCE_OFF, // https://msdn.microsoft.com/en-us/library/aa365184.aspx
        NULL,
        0,
        NULL,
        0
        );

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

BOOLEAN DiskDriveQuerySupported(
    _In_ HANDLE DeviceHandle
    )
{
    IO_STATUS_BLOCK isb;
    BOOLEAN deviceQuerySupported = FALSE;
    STORAGE_HOTPLUG_INFO storHotplugInfo;

    memset(&storHotplugInfo, 0, sizeof(STORAGE_HOTPLUG_INFO));

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_STORAGE_GET_HOTPLUG_INFO, // https://msdn.microsoft.com/en-us/library/aa363408.aspx
        NULL,
        0,
        &storHotplugInfo,
        sizeof(storHotplugInfo)
        )))
    {
        deviceQuerySupported = TRUE;
    }

    return deviceQuerySupported;
}

BOOLEAN DiskDriveQueryImminentFailure(
    _In_ HANDLE DeviceHandle
    )
{
    IO_STATUS_BLOCK isb;
    BOOLEAN deviceQuerySupported = FALSE;
    STORAGE_PREDICT_FAILURE storPredictFailure;

    memset(&storPredictFailure, 0, sizeof(STORAGE_PREDICT_FAILURE));

    if (NT_SUCCESS(NtDeviceIoControlFile(
        DeviceHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        IOCTL_STORAGE_PREDICT_FAILURE, // https://msdn.microsoft.com/en-us/library/ff560587.aspx
        NULL,
        0,
        &storPredictFailure,
        sizeof(storPredictFailure)
        )))
    {
        deviceQuerySupported = TRUE;
    }

    // IOCTL_STORAGE_PREDICT_FAILURE returns a ULONG+ opaque 512-byte vendor-specific information, which
    // in all cases contains SMART attribute information (2 bytes header + 12 bytes each attribute).

    // If ATA pass-through or SMART I/O-controls are unavailable,
    //  IOCTL_STORAGE_QUERY_PROPERTY to read part of the IDENTIFY data and
    //  IOCTL_STORAGE_PREDICT_FAILURE to read SMART status and attributes (without thresholds).
    // This works without admin rights but doesn't support other features like logs and self-tests. It works for (S)ATA devices but not for USB.

    return deviceQuerySupported;
}
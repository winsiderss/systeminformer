/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2020
 *
 */

#include "devices.h"

VOID DiskDriveAddListViewItemGroups(
    _In_ HWND ListViewHandle,
    _In_ INT DiskGroupId
    )
{
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_FS_CREATION_TIME, L"Volume creation time", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_SERIAL_NUMBER, L"Volume serial number", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_FILE_SYSTEM, L"Volume file system", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_FS_VERSION, L"Volume version", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_LFS_VERSION, L"LFS version", NULL);

    //PhAddListViewGroupItem(Context->ListViewHandle, diskGroupId, MAXINT, L"BytesPerPhysicalSector", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_TOTAL_SIZE, L"Total size", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_TOTAL_FREE, L"Total free", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_TOTAL_SECTORS, L"Total sectors", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS, L"Total clusters", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_FREE_CLUSTERS, L"Free clusters", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_TOTAL_RESERVED, L"Reserved clusters", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_SECTOR, L"Bytes per sector", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_CLUSTER, L"Bytes per cluster", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_RECORD, L"Bytes per file record segment", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS_PER_RECORD, L"Clusters per File record segment", NULL);

    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_RECORDS, L"MFT records", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_SIZE, L"MFT size", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_START, L"MFT start", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_ZONE, L"MFT Zone clusters", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_ZONE_SIZE, L"MFT zone size", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_MIRROR_START, L"MFT mirror start", NULL);

    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_FILE_READS, L"File reads", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_FILE_WRITES, L"File writes", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_DISK_READS, L"Disk reads", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_DISK_WRITES, L"Disk writes", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_FILE_READ_BYTES, L"File read bytes", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_FILE_WRITE_BYTES, L"File write bytes", NULL);

    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_METADATA_READS, L"Metadata reads", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_METADATA_WRITES, L"Metadata writes", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_METADATA_DISK_READS, L"Metadata disk reads", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_METADATA_DISK_WRITES, L"Metadata disk writes", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_METADATA_READ_BYTES, L"Metadata read bytes", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_METADATA_WRITE_BYTES, L"Metadata write bytes", NULL);

    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_READS, L"Mft reads", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_WRITES, L"Mft writes", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_READ_BYTES, L"Mft read bytes", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_WRITE_BYTES, L"Mft write bytes", NULL);

    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_READS, L"RootIndex reads", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_WRITES, L"RootIndex writes", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_READ_BYTES, L"RootIndex read bytes", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_WRITE_BYTES, L"RootIndex write bytes", NULL);

    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_BITMAP_READS, L"Bitmap reads", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_BITMAP_WRITES, L"Bitmap writes", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_BITMAP_READ_BYTES, L"Bitmap read bytes", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_BITMAP_WRITE_BYTES, L"Bitmap write bytes", NULL);

    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_READS, L"Mft bitmap reads", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_WRITES, L"Mft bitmap writes", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_READ_BYTES, L"Mft bitmap read bytes", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_WRITE_BYTES, L"Mft bitmap write bytes", NULL);

    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_USER_INDEX_READS, L"User Index reads", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_USER_INDEX_WRITES, L"User Index writes", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_USER_INDEX_READ_BYTES, L"User Index read bytes", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_USER_INDEX_WRITE_BYTES, L"User Index write bytes", NULL);

    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_LOGFILE_READS, L"LogFile reads", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_LOGFILE_WRITES, L"LogFile writes", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_LOGFILE_READ_BYTES, L"LogFile read bytes", NULL);
    //PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_LOGFILE_WRITE_BYTES, L"LogFile write bytes", NULL);

    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, DISKDRIVE_DETAILS_INDEX_OTHER_EXCEPTIONS, L"Exceptions", NULL);
}

VOID DiskDriveQuerySmart(
    _Inout_ PDV_DISK_PAGE_CONTEXT Context
    )
{
    PPH_LIST attributes;
    HANDLE deviceHandle;

    if (NT_SUCCESS(PhCreateFile(
        &deviceHandle,
        &Context->PageContext->DiskId.DevicePath->sr,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        if (NT_SUCCESS(DiskDriveQueryImminentFailure(deviceHandle, &attributes)))
        {
            for (ULONG i = 0; i < attributes->Count; i++)
            {
                PSMART_ATTRIBUTES attribute = attributes->Items[i];

                INT lvItemIndex = PhAddListViewItem(
                    Context->ListViewHandle,
                    MAXINT,
                    SmartAttributeGetText(attribute->AttributeId),
                    IntToPtr(attribute->AttributeId)
                    );

                PhSetListViewSubItem(
                    Context->ListViewHandle,
                    lvItemIndex,
                    1,
                    PhaFormatString(L"%lu", attribute->CurrentValue)->Buffer
                    );
                PhSetListViewSubItem(
                    Context->ListViewHandle,
                    lvItemIndex,
                    2,
                    PhaFormatString(L"%lu", attribute->WorstValue)->Buffer
                    );

                if (attribute->RawValue)
                {
                    PhSetListViewSubItem(
                        Context->ListViewHandle,
                        lvItemIndex,
                        3,
                        PhaFormatString(L"%lu", attribute->RawValue)->Buffer
                        );

                    PhSetListViewSubItem(
                        Context->ListViewHandle,
                        lvItemIndex,
                        4,
                        PhaFormatString(L"%#014x", attribute->RawValue)->Buffer
                        );
                }

                //PhSetListViewSubItem(
                //    Context->ListViewHandle,
                //    lvItemIndex,
                //    5,
                //    PhaFormatString(L"%lu", attribute->Advisory)->Buffer
                //    );
                //PhSetListViewSubItem(
                //    Context->ListViewHandle,
                //    lvItemIndex,
                //    6,
                //    PhaFormatString(L"%lu", attribute->FailureImminent)->Buffer
                //    );

                PhFree(attribute);
            }

            PhDereferenceObject(attributes);
        }

        NtClose(deviceHandle);
    }
}

//COLORREF NTAPI PhpColorItemColorFunction(
//    _In_ INT Index,
//    _In_ PVOID Param,
//    _In_opt_ PVOID Context
//    )
//{
//    PSMART_ATTRIBUTES item = Param;
//
//    if (item->FailureImminent)
//        return RGB(255, 119, 0);// RGB(255, 60, 40);
//
//    return RGB(0xFF, 0xFF, 0xFF);
//}

VOID DiskDriveQueryFileSystem(
    _Inout_ PDV_DISK_PAGE_CONTEXT Context
    )
{
    PPH_LIST deviceMountHandles;

    if (!(deviceMountHandles = DiskDriveQueryMountPointHandles(Context->PageContext->DiskIndex)))
        return;

    for (ULONG i = 0; i < deviceMountHandles->Count; i++)
    {
        USHORT fsInfoType;
        PVOID fsInfoBuffer;
        INT diskGroupId = -1;
        PFILE_FS_VOLUME_INFORMATION volumeInfo;
        PDISK_HANDLE_ENTRY diskEntry;        

        diskEntry = deviceMountHandles->Items[i];

        if (NT_SUCCESS(DiskDriveQueryVolumeInformation(diskEntry->DeviceHandle, &volumeInfo)))
        {
            SYSTEMTIME systemTime;

            PhLargeIntegerToLocalSystemTime(&systemTime, &volumeInfo->VolumeCreationTime);

            if (volumeInfo->VolumeLabelLength > 0)
            {
                diskGroupId = PhAddListViewGroup(Context->ListViewHandle, i, PhaFormatString(
                    L"Volume %wc: [%s]",
                    diskEntry->DeviceLetter,
                    PhaCreateStringEx(volumeInfo->VolumeLabel, volumeInfo->VolumeLabelLength)->Buffer
                    )->Buffer);
            }
            else
            {
                diskGroupId = PhAddListViewGroup(Context->ListViewHandle, i, PhaFormatString(
                    L"Volume %wc:",
                    diskEntry->DeviceLetter
                    )->Buffer);
            }

            DiskDriveAddListViewItemGroups(Context->ListViewHandle, diskGroupId);
            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FS_CREATION_TIME, 1, PhaFormatDateTime(&systemTime)->Buffer);

            PhFree(volumeInfo);
        }
        else
        {
            diskGroupId = PhAddListViewGroup(
                Context->ListViewHandle,
                i,
                PhaFormatString(L"Volume %wc:", diskEntry->DeviceLetter)->Buffer
                );

            DiskDriveAddListViewItemGroups(Context->ListViewHandle, diskGroupId);
        }

        if (DiskDriveQueryFileSystemInfo(diskEntry->DeviceHandle, &fsInfoType, &fsInfoBuffer))
        {
            switch (fsInfoType)
            {
            case FILESYSTEM_STATISTICS_TYPE_NTFS:
            case FILESYSTEM_STATISTICS_TYPE_REFS:
                {                
                    PNTFS_FILESYSTEM_STATISTICS buffer = fsInfoBuffer;

                    if (fsInfoType == FILESYSTEM_STATISTICS_TYPE_NTFS)
                    {
                        NTFS_VOLUME_INFO ntfsVolumeInfo;

                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_SYSTEM, 1, L"NTFS");

                        if (DiskDriveQueryNtfsVolumeInfo(diskEntry->DeviceHandle, &ntfsVolumeInfo))
                        {
                            ntfsVolumeInfo.VolumeData.VolumeSerialNumber.QuadPart = _byteswap_uint64(ntfsVolumeInfo.VolumeData.VolumeSerialNumber.QuadPart);
                            
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_SERIAL_NUMBER, 1,
                                PhaFormatString(L"0x%s", PH_AUTO_T(PH_STRING, PhBufferToHexString((PUCHAR)&ntfsVolumeInfo.VolumeData.VolumeSerialNumber.QuadPart, sizeof(ntfsVolumeInfo.VolumeData.VolumeSerialNumber.QuadPart)))->Buffer)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FS_VERSION, 1,
                                PhaFormatString(L"%lu.%lu", ntfsVolumeInfo.ExtendedVolumeData.MajorVersion, ntfsVolumeInfo.ExtendedVolumeData.MinorVersion)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_LFS_VERSION, 1,
                                PhaFormatString(L"%lu.%lu", ntfsVolumeInfo.ExtendedVolumeData.LfsMajorVersion, ntfsVolumeInfo.ExtendedVolumeData.LfsMinorVersion)->Buffer);
                            //PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1,
                            //    PhaFormatSize(ntfsVolumeInfo.ExtendedVolumeData.BytesPerPhysicalSector, ULONG_MAX)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_SIZE, 1,
                                PhaFormatSize(ntfsVolumeInfo.VolumeData.NumberSectors.QuadPart * ntfsVolumeInfo.VolumeData.BytesPerSector, ULONG_MAX)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_FREE, 1,
                                PhaFormatString(L"%s (%.2f%%)", PhaFormatSize(ntfsVolumeInfo.VolumeData.FreeClusters.QuadPart * ntfsVolumeInfo.VolumeData.BytesPerCluster, ULONG_MAX)->Buffer, (FLOAT)(ntfsVolumeInfo.VolumeData.FreeClusters.QuadPart * 100) / ntfsVolumeInfo.VolumeData.TotalClusters.QuadPart)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_SECTORS, 1,
                                PhaFormatUInt64(ntfsVolumeInfo.VolumeData.NumberSectors.QuadPart, TRUE)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS, 1,
                                PhaFormatUInt64(ntfsVolumeInfo.VolumeData.TotalClusters.QuadPart, TRUE)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FREE_CLUSTERS, 1,
                                PhaFormatUInt64(ntfsVolumeInfo.VolumeData.FreeClusters.QuadPart, TRUE)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_RESERVED, 1,
                                PhaFormatUInt64(ntfsVolumeInfo.VolumeData.TotalReserved.QuadPart, TRUE)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_SECTOR, 1,
                                PhaFormatString(L"%lu", ntfsVolumeInfo.VolumeData.BytesPerSector)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_CLUSTER, 1,
                                PhaFormatString(L"%lu", ntfsVolumeInfo.VolumeData.BytesPerCluster)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_RECORD, 1,
                                PhaFormatString(L"%lu", ntfsVolumeInfo.VolumeData.BytesPerFileRecordSegment)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS_PER_RECORD, 1,
                                PhaFormatString(L"%lu", ntfsVolumeInfo.VolumeData.ClustersPerFileRecordSegment)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_RECORDS, 1,
                                PhaFormatUInt64(ntfsVolumeInfo.VolumeData.MftValidDataLength.QuadPart / ntfsVolumeInfo.VolumeData.BytesPerFileRecordSegment, TRUE)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_SIZE, 1,
                                PhaFormatString(L"%s (%.2f%%)", PhaFormatSize(ntfsVolumeInfo.VolumeData.MftValidDataLength.QuadPart, ULONG_MAX)->Buffer, (FLOAT)(ntfsVolumeInfo.VolumeData.MftValidDataLength.QuadPart * 100 / ntfsVolumeInfo.VolumeData.BytesPerCluster) / ntfsVolumeInfo.VolumeData.TotalClusters.QuadPart)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_START, 1,
                                PhaFormatString(L"%I64u", ntfsVolumeInfo.VolumeData.MftStartLcn.QuadPart)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_ZONE, 1,
                                PhaFormatString(L"%I64u - %I64u", ntfsVolumeInfo.VolumeData.MftZoneStart.QuadPart, ntfsVolumeInfo.VolumeData.MftZoneEnd.QuadPart)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_ZONE_SIZE, 1,
                                PhaFormatString(L"%s (%.2f%%)", PhaFormatSize((ntfsVolumeInfo.VolumeData.MftZoneEnd.QuadPart - ntfsVolumeInfo.VolumeData.MftZoneStart.QuadPart) * ntfsVolumeInfo.VolumeData.BytesPerCluster, ULONG_MAX)->Buffer, (FLOAT)(ntfsVolumeInfo.VolumeData.MftZoneEnd.QuadPart - ntfsVolumeInfo.VolumeData.MftZoneStart.QuadPart) * 100 / ntfsVolumeInfo.VolumeData.TotalClusters.QuadPart)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_MIRROR_START, 1,
                                PhaFormatString(L"%I64u", ntfsVolumeInfo.VolumeData.Mft2StartLcn.QuadPart)->Buffer);
                        }
                    }
                    else
                    {
                        REFS_VOLUME_DATA_BUFFER refsVolumeInfo;

                        PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_SYSTEM, 1, L"ReFS");

                        if (DiskDriveQueryRefsVolumeInfo(diskEntry->DeviceHandle, &refsVolumeInfo))
                        {
                            refsVolumeInfo.VolumeSerialNumber.QuadPart = _byteswap_uint64(refsVolumeInfo.VolumeSerialNumber.QuadPart);

                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_SERIAL_NUMBER, 1,
                                PhaFormatString(L"0x%s", PH_AUTO_T(PH_STRING, PhBufferToHexString((PUCHAR)&refsVolumeInfo.VolumeSerialNumber.QuadPart, sizeof(LONGLONG)))->Buffer)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FS_VERSION, 1,
                                PhaFormatString(L"%lu.%lu", refsVolumeInfo.MajorVersion, refsVolumeInfo.MinorVersion)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_SIZE, 1,
                                PhaFormatSize(refsVolumeInfo.NumberSectors.QuadPart * refsVolumeInfo.BytesPerSector, ULONG_MAX)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_FREE, 1,
                                PhaFormatString(L"%s (%.2f%%)", PhaFormatSize(refsVolumeInfo.FreeClusters.QuadPart * refsVolumeInfo.BytesPerCluster, ULONG_MAX)->Buffer, (FLOAT)(refsVolumeInfo.FreeClusters.QuadPart * 100) / refsVolumeInfo.TotalClusters.QuadPart)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_SECTORS, 1,
                                PhaFormatUInt64(refsVolumeInfo.NumberSectors.QuadPart, TRUE)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS, 1,
                                PhaFormatUInt64(refsVolumeInfo.TotalClusters.QuadPart, TRUE)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FREE_CLUSTERS, 1,
                                PhaFormatUInt64(refsVolumeInfo.FreeClusters.QuadPart, TRUE)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_RESERVED, 1,
                                PhaFormatUInt64(refsVolumeInfo.TotalReserved.QuadPart, TRUE)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_SECTOR, 1,
                                PhaFormatUInt64(refsVolumeInfo.BytesPerSector, TRUE)->Buffer);
                            PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_CLUSTER, 1,
                                PhaFormatUInt64(refsVolumeInfo.BytesPerCluster, TRUE)->Buffer);
                        }
                    }

                    // File System

                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_READS, 1,
                        PhaFormatUInt64(buffer->FileSystemStatistics.UserFileReads, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_WRITES, 1,
                        PhaFormatUInt64(buffer->FileSystemStatistics.UserFileWrites, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_DISK_READS, 1,
                        PhaFormatUInt64(buffer->FileSystemStatistics.UserDiskReads, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_DISK_WRITES, 1,
                        PhaFormatUInt64(buffer->FileSystemStatistics.UserDiskWrites, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_READ_BYTES, 1,
                        PhaFormatSize(buffer->FileSystemStatistics.UserFileReadBytes, ULONG_MAX)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_FILE_WRITE_BYTES, 1,
                        PhaFormatSize(buffer->FileSystemStatistics.UserFileWriteBytes, ULONG_MAX)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_READS, 1,
                        PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataReads, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_WRITES, 1,
                        PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataWrites, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_DISK_READS, 1,
                        PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataDiskReads, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_DISK_WRITES, 1,
                        PhaFormatUInt64(buffer->FileSystemStatistics.MetaDataDiskWrites, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_READ_BYTES, 1,
                        PhaFormatSize(buffer->FileSystemStatistics.MetaDataReadBytes, ULONG_MAX)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_METADATA_WRITE_BYTES, 1,
                        PhaFormatSize(buffer->FileSystemStatistics.MetaDataWriteBytes, ULONG_MAX)->Buffer);

                    // NTFS specific 

                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_READS, 1,
                        PhaFormatUInt64(buffer->NtfsStatistics.MftReads, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_WRITES, 1,
                        PhaFormatUInt64(buffer->NtfsStatistics.MftWrites, TRUE)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_READ_BYTES, 1,
                        PhaFormatSize(buffer->NtfsStatistics.MftReadBytes, ULONG_MAX)->Buffer);
                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_MFT_WRITE_BYTES, 1,
                        PhaFormatSize(buffer->NtfsStatistics.MftWriteBytes, ULONG_MAX)->Buffer);

                    PhSetListViewSubItem(Context->ListViewHandle, DISKDRIVE_DETAILS_INDEX_OTHER_EXCEPTIONS, 1,
                        PhaFormatUInt64(UInt32Add32To64(buffer->NtfsStatistics.LogFileFullExceptions, buffer->NtfsStatistics.OtherExceptions), TRUE)->Buffer);

                    // TODO: Additions for Windows 8.1 and Windows 10...
                }
                break;
            case FILESYSTEM_STATISTICS_TYPE_FAT:
                {
                    //PFAT_FILESYSTEM_STATISTICS buffer = fsInfoBuffer;
                }
                break;
            case FILESYSTEM_STATISTICS_TYPE_EXFAT:
                {
                    //PEXFAT_FILESYSTEM_STATISTICS buffer = fsInfoBuffer;
                }
                break;
            }

            PhFree(fsInfoBuffer);
        }

        NtClose(diskEntry->DeviceHandle);
        PhFree(diskEntry);
    }
}

INT_PTR CALLBACK DiskDriveFileSystemDetailsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_PAGE_CONTEXT context = NULL;
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(DV_DISK_PAGE_CONTEXT));
        context->PageContext = (PCOMMON_PAGE_CONTEXT)propPageContext->Context;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_DISK_COUNTERS_COLUMNS, context->ListViewHandle);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
            context = NULL;
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = context->PageContext->SysInfoContext->DetailsWindowDialogHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DETAILS_LIST);

            PhCenterWindow(GetParent(hwndDlg), NULL); // HACK (dmex)
            PhSetApplicationWindowIcon(GetParent(hwndDlg));
     
            if (!!PhGetIntegerSetting(L"EnableThemeSupport")) // TODO: Required for compat (dmex)
                PhInitializeWindowTheme(GetParent(hwndDlg), !!PhGetIntegerSetting(L"EnableThemeSupport"));
            else
                PhInitializeWindowTheme(hwndDlg, FALSE);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 290, L"Property");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 130, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_DISK_COUNTERS_COLUMNS, context->ListViewHandle);

            DiskDriveQueryFileSystem(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_PH_SHOW_DIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK DiskDriveSmartDetailsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_PAGE_CONTEXT context = NULL;
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(DV_DISK_PAGE_CONTEXT));
        context->PageContext = (PCOMMON_PAGE_CONTEXT)propPageContext->Context;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhSaveListViewColumnsToSetting(SETTING_NAME_SMART_COUNTERS_COLUMNS, context->ListViewHandle);

            PhFree(context);
            context = NULL;
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = context->PageContext->SysInfoContext->DetailsWindowDialogHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DETAILS_LIST);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 240, L"Property");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 50, L"Value");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"Best");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Raw");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"Raw (Hex)");
            //PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"Advisory");
            //PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 80, L"Failure Imminent");
            PhSetExtendedListView(context->ListViewHandle);
            //ExtendedListView_SetItemColorFunction(context->ListViewHandle, PhpColorItemColorFunction);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_SMART_COUNTERS_COLUMNS, context->ListViewHandle);

            DiskDriveQuerySmart(context);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_PH_SHOW_DIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    }

    return FALSE;
}

VOID FreeDiskDriveDetailsContext(
    _In_ PCOMMON_PAGE_CONTEXT Context
    )
{
    if (Context->DeviceHandle)
        NtClose(Context->DeviceHandle);

    DeleteDiskId(&Context->DiskId);
    PhClearReference(&Context->DiskName);
    PhFree(Context);
}

NTSTATUS ShowDiskDriveDetailsDialogThread(
    _In_ PVOID Parameter
    )
{
    PPV_PROPCONTEXT propContext;
    PCOMMON_PAGE_CONTEXT pageContext = Parameter;

    PhSetEvent(&pageContext->SysInfoContext->DetailsWindowInitializedEvent);

    if (propContext = HdCreatePropContext(L"Disk Drive"))
    {
        PPV_PROPPAGECONTEXT newPage;

        // FileSystem page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_DISKDRIVE_DETAILS_FILESYSTEM),
            DiskDriveFileSystemDetailsDlgProc,
            pageContext);
        PvAddPropPage(propContext, newPage);

        // Smart page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_DISKDRIVE_DETAILS_SMART),
            DiskDriveSmartDetailsDlgProc,
            pageContext);
        PvAddPropPage(propContext, newPage);

        PhModalPropertySheet(&propContext->PropSheetHeader);
        PhDereferenceObject(propContext);
    }

    PhResetEvent(&pageContext->SysInfoContext->DetailsWindowInitializedEvent);

    if (pageContext->SysInfoContext->DetailsWindowThreadHandle)
    {
        NtClose(pageContext->SysInfoContext->DetailsWindowThreadHandle);
        pageContext->SysInfoContext->DetailsWindowThreadHandle = NULL;
    }

    return STATUS_SUCCESS;
}

VOID ShowDiskDriveDetailsDialog(
    _In_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    if (!Context->DetailsWindowThreadHandle)
    {
        HANDLE threadHandle;
        PCOMMON_PAGE_CONTEXT pageContext;

        pageContext = PhAllocateZero(sizeof(COMMON_PAGE_CONTEXT));
        pageContext->ParentHandle = GetParent(GetParent(Context->WindowHandle));
        pageContext->SysInfoContext = Context;
        pageContext->DiskIndex = Context->DiskEntry->DiskIndex;
        //pageContext->Length = Context->DiskEntry->DiskLength;

        PhSetReference(&pageContext->DiskName, Context->DiskEntry->DiskName);
        CopyDiskId(&pageContext->DiskId, &Context->DiskEntry->Id);

        if (!NT_SUCCESS(PhCreateThreadEx(&threadHandle, ShowDiskDriveDetailsDialogThread, pageContext)))
        {
            PhShowError(Context->WindowHandle, L"%s", L"Unable to create the window.");
            return;
        }

        PhWaitForEvent(&Context->DetailsWindowInitializedEvent, NULL);

        Context->DetailsWindowThreadHandle = threadHandle;
    }

    PostMessage(Context->DetailsWindowDialogHandle, WM_PH_SHOW_DIALOG, 0, 0);
}

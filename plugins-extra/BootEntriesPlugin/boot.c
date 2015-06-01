/*
 * Process Hacker Extra Plugins -
 *   Boot Entries Plugin
 *
 * Copyright (C) 2015 dmex
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

#include "main.h"

#include <stdint.h>
#include "Efi\EfiTypes.h"
#include "Efi\EfiDevicePath.h"

_NtAddBootEntry NtAddBootEntry_I = NULL;
_NtDeleteBootEntry NtDeleteBootEntry_I = NULL;
_NtModifyBootEntry NtModifyBootEntry_I = NULL;
_NtEnumerateBootEntries NtEnumerateBootEntries_I = NULL;
_NtQueryBootEntryOrder NtQueryBootEntryOrder_I = NULL;
_NtSetBootEntryOrder NtSetBootEntryOrder_I = NULL;
_NtQueryBootOptions NtQueryBootOptions_I = NULL;
_NtSetBootOptions NtSetBootOptions_I = NULL;
_NtTranslateFilePath NtTranslateFilePath_I = NULL;

static BOOLEAN BootEntryCallback(
    _In_ PBOOT_ENTRY BootEntry,
    _In_ PVOID Context
    )
{
    HWND listViewHandle = (HWND)Context;
    PWSTR bootEntryName = (PWSTR)((PBYTE)BootEntry + BootEntry->FriendlyNameOffset);
    PFILE_PATH bootFilePath = (PFILE_PATH)((PBYTE)BootEntry + BootEntry->BootFilePathOffset);

    PPH_STRING bootEntryID = PhFormatString(L"%lu", BootEntry->Id);
    INT index = PhAddListViewItem(
        listViewHandle,
        MAXINT,
        bootEntryID->Buffer,
        bootEntryID->Buffer
        );
    PhDereferenceObject(bootEntryID);

    PhSetListViewSubItem(
        listViewHandle,
        index,
        1,
        bootEntryName
        );

    if (bootFilePath->Type == FILE_PATH_TYPE_EFI)
    {
        EFI_DEV_PATH* efiDevPath = (EFI_DEV_PATH*)bootFilePath->FilePath;
        EFI_DEVICE_PATH_PROTOCOL efiDevPathProtocol = efiDevPath->DevPath;

        if (efiDevPathProtocol.Type == MEDIA_DEVICE_PATH)
        {
            //HARDDRIVE_DEVICE_PATH hardDriveDevicePath = efiDevPath->HardDrive;

            //PPH_STRING partitionLength = PhFormatSize(hardDriveDevicePath.PartitionSize, -1);
            //if (hardDriveDevicePath.SignatureType == SIGNATURE_TYPE_GUID)
            //PPH_STRING guidString = PhFormatGuid(hardDriveDevicePath.Signature);

            PhSetListViewSubItem(listViewHandle, index, 2, L"MEDIA_DEVICE_PATH");
        }
        else if (efiDevPathProtocol.Type == ACPI_DEVICE_PATH)
        {
            if (efiDevPathProtocol.SubType == ACPI_DP)
            {
                //ACPI_HID_DEVICE_PATH vendorPath1 = efiDevPath->Acpi;
            }
            else if (efiDevPathProtocol.SubType == ACPI_EXTENDED_DP)
            {
                //ACPI_EXTENDED_HID_DEVICE_PATH extendedAcpi = efiDevPath->ExtendedAcpi;
            }
            else if (efiDevPathProtocol.SubType == ACPI_ADR_DP)
            {
                //ACPI_ADR_DEVICE_PATH adrAcpi = efiDevPath->AdrAcpi;
            }
                   
            PhSetListViewSubItem(listViewHandle, index, 2, L"ACPI_DEVICE_PATH");
        }
        else if (efiDevPathProtocol.Type == BBS_DEVICE_PATH)
        {
            if (efiDevPathProtocol.SubType == BBS_BBS_DP)
            {
                // BIOS Boot Specification
                //BBS_BBS_DEVICE_PATH vendorPath = efiDevPath->Bbs;

                //union
                //{
                //    USHORT StatusFlag;
                //    struct
                //    {
                //        USHORT OldPosition : 3;    // This entry’s index in the table at the last boot. For updating the IPL or BCV Priority if individual device detection is done.
                //        USHORT Reserved1 : 7;      // Reserved for future use, must be zero.
                //        USHORT Enabled : 8;        // 0 = Entry will be ignored for booting (IPL); entry will not be called for boot connection (BCV). 1 = Entry will be attempted for booting (IPL); entry will be called for boot connection (BCV).
                //        USHORT Failed : 9;         // 0 = Has not been attempted for boot, or it is unknown if boot failure occurred (IPL); entry connected successfully (BCV). 1 = Failed boot attempt (IPL); failed connection attempt (BCV).
                //        USHORT MediaPresent : 11;  // 0 = No bootable media present in the device. 1 = Unknown if bootable media present. 2 = Media present and appears bootable. 3 = Reserved for future use.
                //        USHORT Reserved2 : 15;     // Reserved for future use, must be zero.
                //    };
                //} DeviceStatusFlag = { vendorPath.StatusFlag };

                //if (vendorPath.DeviceType == BBS_TYPE_HARDDRIVE) { }
            }

            PhSetListViewSubItem(listViewHandle, index, 2, L"BIOS Boot Specification");
        }
        else
        {
            PhSetListViewSubItem(listViewHandle, index, 2, L"[Unknown]");
        }
    }

    return TRUE;
}

NTSTATUS PhEnumerateBootEntries(    
    _In_ PPH_BOOT_ENTRY_CALLBACK BootEntryCallback,
    _In_opt_ PVOID Context
    )
{
    ULONG bufferLength = 0;
    PVOID buffer = NULL;

    __try
    {
        if (!NtEnumerateBootEntries_I)
            __leave;

        if (NtEnumerateBootEntries_I(NULL, &bufferLength) != STATUS_BUFFER_TOO_SMALL)
            __leave;

        buffer = PhAllocate(bufferLength);

        if (NT_SUCCESS(NtEnumerateBootEntries_I(buffer, &bufferLength)))
        {
            PBOOT_ENTRY_LIST bootEntryList = buffer;

            do
            {
                if (!BootEntryCallback(&bootEntryList->BootEntry, Context))
                    break;

            } while (bootEntryList->NextEntryOffset && (bootEntryList = (PBOOT_ENTRY_LIST)PTR_ADD_OFFSET(bootEntryList, bootEntryList->NextEntryOffset)));
        }
    }
    __finally
    {
        if (buffer)
        {
            PhFree(buffer);
        }
    }

    return STATUS_SUCCESS;
}

VOID QueryBootOptions(
    VOID
    )
{
    ULONG bufferLength = 0;
    PVOID buffer = NULL;

    __try
    {
        if (NtQueryBootOptions_I(NULL, &bufferLength) != STATUS_BUFFER_TOO_SMALL)
            __leave;

        buffer = PhAllocate(bufferLength);

        if (NT_SUCCESS(NtQueryBootOptions_I(buffer, &bufferLength)))
        {
            PBOOT_OPTIONS bootOptions = buffer;
        }
    }
    __finally
    {
        if (buffer)
        {
            PhFree(buffer);
        }
    }
}

NTSTATUS EnumerateBootEntriesThread(
    _In_ PVOID ThreadParam
    )
{
    HANDLE tokenHandle;

    // Enable required privileges
    if (NT_SUCCESS(PhOpenProcessToken(
        &tokenHandle,
        TOKEN_ADJUST_PRIVILEGES,
        NtCurrentProcess()
        )))
    {
        PhSetTokenPrivilege(tokenHandle, SE_SYSTEM_ENVIRONMENT_NAME, NULL, SE_PRIVILEGE_ENABLED);
        NtClose(tokenHandle);
    }

    //QueryBootOptions();
    PhEnumerateBootEntries(BootEntryCallback, ThreadParam);

    return STATUS_SUCCESS;
}

BOOLEAN IsUefiSystem(
    VOID
    )
{
    // Windows 8
    //FIRMWARE_TYPE fwtype;
    //GetFirmwareType(&fwtype);
    //if (fwtype == FirmwareTypeUefi)
    //{
    //    return TRUE;
    //}

    if (GetFirmwareEnvironmentVariable(L"", L"{00000000-0000-0000-0000-000000000000}", NULL, 0) == ERROR_INVALID_FUNCTION)
    {
        return TRUE;
    }

    return FALSE;
}
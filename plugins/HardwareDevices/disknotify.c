/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2016-2018 dmex
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
#include <dbt.h>

BOOLEAN MainWndDeviceChangeRegistrationEnabled = FALSE;
PH_CALLBACK_REGISTRATION MainWndDeviceChangeEventRegistration;

VOID NTAPI HardwareDevicesDeviceChangeCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PMSG message = Parameter;

    if (!message)
        return;

    switch (message->wParam)
    {
    case DBT_DEVICEARRIVAL: // Drive letter added
    case DBT_DEVICEREMOVECOMPLETE: // Drive letter removed
        {
            DEV_BROADCAST_HDR* deviceBroadcast = (DEV_BROADCAST_HDR*)message->lParam;

            if (deviceBroadcast->dbch_devicetype == DBT_DEVTYP_VOLUME)
            {
                //PDEV_BROADCAST_VOLUME deviceVolume = (PDEV_BROADCAST_VOLUME)deviceBroadcast;

                PhAcquireQueuedLockShared(&DiskDrivesListLock);

                for (ULONG i = 0; i < DiskDrivesList->Count; i++)
                {
                    PDV_DISK_ENTRY entry;

                    entry = PhReferenceObjectSafe(DiskDrivesList->Items[i]);

                    if (!entry)
                        continue;

                    // Reset the DiskIndex so we can re-query the index on the next interval update.
                    entry->DiskIndex = ULONG_MAX;
                    // Reset the DiskIndexName so we can re-query the name on the next interval update.
                    PhClearReference(&entry->DiskIndexName);

                    PhDereferenceObjectDeferDelete(entry);
                }

                PhReleaseQueuedLockShared(&DiskDrivesListLock);
            }
        }
    }
}

VOID AddRemoveDeviceChangeCallback(
    VOID
    )
{
    // We get called during the plugin LoadCallback, don't do anything.
    if (!PhMainWndHandle)
        return;

    // Add the subclass only when disks are being monitored, remove when no longer needed.
    if (DiskDrivesList->Count)
    {
        if (!MainWndDeviceChangeRegistrationEnabled)
        {
            // We have a disk device, register for window events needed to detect drive letter changes.
            MainWndDeviceChangeRegistrationEnabled = TRUE;
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent),
                HardwareDevicesDeviceChangeCallback,
                NULL,
                &MainWndDeviceChangeEventRegistration
                );
        }
    }
    else
    {
        if (MainWndDeviceChangeRegistrationEnabled)
        {
            // Remove the event callback after the user has removed the last disk device.
            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent), &MainWndDeviceChangeEventRegistration);
            MainWndDeviceChangeRegistrationEnabled = FALSE;
        }
    }
}

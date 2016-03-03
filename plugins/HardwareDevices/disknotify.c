/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2016 dmex
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
#include <Dbt.h>

static BOOLEAN SubclassActive = FALSE;

LRESULT CALLBACK MainWndDevicesSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    )
{ 
    // Subclassing the main window just to process drive letter notifications 
    // is bad and I don't know of any other way to achieve this.
    // The IOCTL_MOUNTMGR_CHANGE_NOTIFY callback would have been preferred but 
    // doesn't work from non-elevated processes.

    switch (uMsg)
    {
    case WM_DEVICECHANGE:
        {
            switch (wParam)
            {
            case DBT_DEVICEARRIVAL: // Drive letter added
            case DBT_DEVICEREMOVECOMPLETE: // Drive letter removed
                {
                    DEV_BROADCAST_HDR* deviceBroadcast = (DEV_BROADCAST_HDR*)lParam;

                    if (deviceBroadcast->dbch_devicetype == DBT_DEVTYP_VOLUME)
                    {
                        PDEV_BROADCAST_VOLUME deviceVolume = (PDEV_BROADCAST_VOLUME)deviceBroadcast;

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

            goto DefaultWndProc;
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

VOID AddRemoveDeviceChangeCallback(
    VOID
    )
{
    // We get called during the plugin LoadCallback, don't do anything.
    if (!PhMainWndHandle)
        return;

    // Add the subclass only when disks are being monitored, remove when no longer needed.
    if (DiskDrivesList->Count != 0)
    {
        if (!SubclassActive)
        {
            // We have a disk device, subclass the main window to detect drive letter changes.
            SetWindowSubclass(PhMainWndHandle, MainWndDevicesSubclassProc, 0, 0);
            SubclassActive = TRUE;
        }
    }
    else
    {
        if (SubclassActive)
        {
            // The user has removed the last disk device, remove the subclass.
            RemoveWindowSubclass(PhMainWndHandle, MainWndDevicesSubclassProc, 0);
            SubclassActive = FALSE;
        }
    }
}
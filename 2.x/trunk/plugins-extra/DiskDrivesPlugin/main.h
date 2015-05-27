/*
 * Process Hacker Extra Plugins -
 *   Disk Drives Plugin
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

#ifndef _DISK_H_
#define _DISK_H_

#define PLUGIN_NAME L"dmex.DiskDrivesPlugin"
#define SETTING_NAME_DISK_LIST (PLUGIN_NAME L".DiskList")

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>

#include "resource.h"

extern PPH_PLUGIN PluginInstance;
extern PPH_LIST DiskDrivesList;

typedef struct _PH_DISK_ENTRY
{
    PPH_STRING DiskFriendlyName;
    PPH_STRING DiskDevicePath;
} PH_DISK_ENTRY, *PPH_DISK_ENTRY;

typedef struct _PH_DISKDRIVE_CONTEXT
{
    HWND ListViewHandle;
    PPH_LIST DiskDrivesListEdited;
} PH_DISKDRIVE_CONTEXT, *PPH_DISKDRIVE_CONTEXT;

typedef struct _PH_DISKDRIVE__SYSINFO_CONTEXT
{
    BOOLEAN HaveFirstSample;

    ULONG64 BytesReadValue;
    ULONG64 BytesWriteValue;
    ULONG64 LastBytesReadValue;
    ULONG64 LastBytesWriteValue;
    ULONG64 LastReadTime;
    ULONG64 LastWriteTime;
    ULONG64 LastIdletime;
    ULONG64 LastQueryTime;

    ULONG64 ResponseTime;
    FLOAT ActiveTime;
    ULONG QueueDepth;
    ULONG SplitCount;

    //PPH_STRING DiskName;
    PPH_STRING DiskLength;
    PPH_DISK_ENTRY DiskEntry;

    HANDLE DeviceHandle;
    
    HWND WindowHandle;
    HWND PanelWindowHandle;
    HWND GraphHandle;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_GRAPH_STATE GraphState;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    PH_CIRCULAR_BUFFER_ULONG64 ReadBuffer;
    PH_CIRCULAR_BUFFER_ULONG64 WriteBuffer;
} PH_DISKDRIVE_SYSINFO_CONTEXT, *PPH_DISKDRIVE_SYSINFO_CONTEXT;

INT_PTR CALLBACK DiskViewDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID LoadDiskDriveList(
    _Inout_ PPH_LIST FilterList,
    _In_ PPH_STRING String
    );

VOID ShowOptionsDialog(
    _In_ HWND ParentHandle
    );

VOID DiskDriveSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ PPH_DISK_ENTRY DiskEntry
    );



typedef VOID (NTAPI *PPH_DISK_ENUM_CALLBACK)(
    _In_ PWSTR DevicePath,
    _In_ PWSTR FriendlyName,
    _In_ PVOID Context
    );

VOID EnumerateDiskDrives(
    _In_ PPH_DISK_ENUM_CALLBACK DiskEnumCallback,
    _In_opt_ PVOID Context
    );

BOOLEAN DiskDriveQueryDeviceInformation(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PPH_STRING* DiskVendor,
    _Out_opt_ PPH_STRING* DiskModel,
    _Out_opt_ PPH_STRING* DiskRevision,
    _Out_opt_ PPH_STRING* DiskSerial
    );

NTSTATUS DiskDriveQueryDeviceTypeAndNumber(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PULONG DeviceNumber,
    _Out_opt_ DEVICE_TYPE* DeviceType
    );

PPH_STRING DiskDriveQueryGeometry(
    _In_ HANDLE DeviceHandle
    );

NTSTATUS DiskDriveQueryStatistics(
    _In_ HANDLE DeviceHandle,
    _Out_ PDISK_PERFORMANCE Info
    );

#endif _DISK_H_
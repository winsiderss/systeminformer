/*
 * Process Hacker Extended Tools -
 *   GPU details window
 *
 * Copyright (C) 2018 dmex
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

#include "exttools.h"
#include "gpumon.h"
#include <uxtheme.h>

typedef enum _NETADAPTER_DETAILS_INDEX
{
    NETADAPTER_DETAILS_INDEX_STATE,
    NETADAPTER_DETAILS_INDEX_IPADDRESS,
    NETADAPTER_DETAILS_INDEX_SUBNET,
    NETADAPTER_DETAILS_INDEX_GATEWAY,
    NETADAPTER_DETAILS_INDEX_DNS,
    NETADAPTER_DETAILS_INDEX_DOMAIN,
} NETADAPTER_DETAILS_INDEX;

VOID EtpGpuDetailsAddListViewItemGroups(
    _In_ HWND ListViewHandle,
    _In_ INT DiskGroupId)
{
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, NETADAPTER_DETAILS_INDEX_STATE, L"AdapterString", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, NETADAPTER_DETAILS_INDEX_IPADDRESS, L"BiosString", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, NETADAPTER_DETAILS_INDEX_SUBNET, L"ChipType", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, NETADAPTER_DETAILS_INDEX_GATEWAY, L"DacType", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, NETADAPTER_DETAILS_INDEX_DNS, L"Driver Model", NULL);
    PhAddListViewGroupItem(ListViewHandle, DiskGroupId, NETADAPTER_DETAILS_INDEX_DOMAIN, L"Driver Version", NULL);
}

VOID EtpQueryAdapterDeviceProperties(
    _In_ PWSTR DeviceName,
    _In_ HWND ListViewHandle)
{
    PPH_STRING driverDate;
    PPH_STRING driverVersion;
    PPH_STRING locationInfo;

    if (EtQueryDeviceProperties(DeviceName, NULL, &driverDate, &driverVersion, &locationInfo, NULL))
    {
        PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_STATE, 1, PhGetStringOrEmpty(driverDate));
        PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_IPADDRESS, 1, PhGetStringOrEmpty(driverVersion));
        PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_SUBNET, 1, PhGetStringOrEmpty(locationInfo));

        PhClearReference(&driverVersion);
        PhClearReference(&driverDate);
    }
}

VOID EtpQueryAdapterRegistryInfo(
    _In_ D3DKMT_HANDLE AdapterHandle, 
    _In_ HWND ListViewHandle)
{
    D3DKMT_ADAPTERREGISTRYINFO adapterInfo;

    memset(&adapterInfo, 0, sizeof(D3DKMT_ADAPTERREGISTRYINFO));

    if (NT_SUCCESS(EtQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_ADAPTERREGISTRYINFO,
        &adapterInfo,
        sizeof(D3DKMT_ADAPTERREGISTRYINFO)
        )))
    {
        //PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_STATE, 1, adapterInfo.AdapterString);
        //PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_IPADDRESS, 1, adapterInfo.BiosString);
        //PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_SUBNET, 1, adapterInfo.ChipType);
        PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_GATEWAY, 1, adapterInfo.DacType);
    }
}

VOID EtpQueryAdapterDriverModel(
    _In_ D3DKMT_HANDLE AdapterHandle, 
    _In_ HWND ListViewHandle)
{
    D3DKMT_DRIVERVERSION wddmversion;

    memset(&wddmversion, 0, sizeof(D3DKMT_DRIVERVERSION));

    if (NT_SUCCESS(EtQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_DRIVERVERSION,
        &wddmversion,
        sizeof(D3DKMT_DRIVERVERSION)
        )))
    {
        switch (wddmversion)
        {
        case KMT_DRIVERVERSION_WDDM_1_0:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"WDDM 1.0");
            break;
        case KMT_DRIVERVERSION_WDDM_1_1_PRERELEASE:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"WDDM 1.1 (pre-release)");
            break;
        case KMT_DRIVERVERSION_WDDM_1_1:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"WDDM 1.1");
            break;
        case KMT_DRIVERVERSION_WDDM_1_2:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"WDDM 1.2");
            break;
        case KMT_DRIVERVERSION_WDDM_1_3:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"WDDM 1.3");
            break;
        case KMT_DRIVERVERSION_WDDM_2_0:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"WDDM 2.0");
            break;
        case KMT_DRIVERVERSION_WDDM_2_1:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"WDDM 2.1");
            break;
        case KMT_DRIVERVERSION_WDDM_2_2:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"WDDM 2.2");
            break;
        case KMT_DRIVERVERSION_WDDM_2_3:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"WDDM 2.3");
            break;
        case KMT_DRIVERVERSION_WDDM_2_4:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"WDDM 2.4");
            break;
        default:
            PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DNS, 1, L"ERROR");
            break;
        }
    }
}

VOID EtpQueryAdapterDriverVersion(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ HWND ListViewHandle)
{
    D3DKMT_UMD_DRIVER_VERSION driverUserVersion;
    D3DKMT_KMD_DRIVER_VERSION driverKernelVersion;

    memset(&driverUserVersion, 0, sizeof(D3DKMT_UMD_DRIVER_VERSION));
    memset(&driverKernelVersion, 0, sizeof(D3DKMT_KMD_DRIVER_VERSION));

    if (NT_SUCCESS(EtQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_UMD_DRIVER_VERSION,
        &driverUserVersion,
        sizeof(D3DKMT_UMD_DRIVER_VERSION)
        )))
    {
        PPH_STRING driverVersionString = PhFormatString(
            L"%hu.%hu.%hu.%hu", 
            HIWORD(driverUserVersion.DriverVersion.HighPart),
            LOWORD(driverUserVersion.DriverVersion.HighPart),
            HIWORD(driverUserVersion.DriverVersion.LowPart),
            LOWORD(driverUserVersion.DriverVersion.LowPart)
            );
        PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DOMAIN, 1, driverVersionString->Buffer);
        PhDereferenceObject(driverVersionString);
    }

    if (NT_SUCCESS(EtQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_KMD_DRIVER_VERSION,
        &driverKernelVersion,
        sizeof(D3DKMT_KMD_DRIVER_VERSION)
        )))
    {
        PPH_STRING driverVersionString = PhFormatString(
            L"%hu.%hu.%hu.%hu",
            HIWORD(driverKernelVersion.DriverVersion.HighPart),
            LOWORD(driverKernelVersion.DriverVersion.HighPart),
            HIWORD(driverKernelVersion.DriverVersion.LowPart),
            LOWORD(driverKernelVersion.DriverVersion.LowPart)
            );
        PhSetListViewSubItem(ListViewHandle, NETADAPTER_DETAILS_INDEX_DOMAIN, 1, driverVersionString->Buffer);
        PhDereferenceObject(driverVersionString);
    }
}

VOID EtpQueryAdapterDeviceIds(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ HWND ListViewHandle)
{
    D3DKMT_QUERY_DEVICE_IDS adapterDeviceId;

    memset(&adapterDeviceId, 0, sizeof(D3DKMT_QUERY_DEVICE_IDS));

    if (NT_SUCCESS(EtQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_PHYSICALADAPTERDEVICEIDS,
        &adapterDeviceId,
        sizeof(D3DKMT_QUERY_DEVICE_IDS)
        )))
    {
        //UINT32 VendorID;
        //UINT32 DeviceID;
        //UINT32 SubVendorID;
        //UINT32 SubSystemID;
        //UINT32 RevisionID;
        //UINT32 BusType;
    }
}

VOID EtpQueryAdapterPerfInfo(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ HWND ListViewHandle)
{
    D3DKMT_ADAPTER_PERFDATA adapterPerfData;

    memset(&adapterPerfData, 0, sizeof(D3DKMT_ADAPTER_PERFDATA));

    if (NT_SUCCESS(EtQueryAdapterInformation(
        AdapterHandle,
        KMTQAITYPE_ADAPTERPERFDATA,
        &adapterPerfData,
        sizeof(D3DKMT_ADAPTER_PERFDATA)
        )))
    {
        //_Out_ ULONGLONG MemoryFrequency; // Clock frequency of the memory in hertz
        //_Out_ ULONGLONG MaxMemoryFrequency; // Max clock frequency of the memory while not overclocked, represented in hertz.
        //_Out_ ULONGLONG MaxMemoryFrequencyOC; // Clock frequency of the memory while overclocked in hertz.
        //_Out_ ULONGLONG MemoryBandwidth; // Amount of memory transferred in bytes
        //_Out_ ULONGLONG PCIEBandwidth; // Amount of memory transferred over PCI-E in bytes
        //_Out_ ULONG FanRPM; // Fan rpm
        //_Out_ ULONG Power; // Power draw of the adapter in tenths of a percentage
        //_Out_ ULONG Temperature; // Temperature in deci-Celsius 1 = 0.1C
        //_Out_ UCHAR PowerStateOverride; // Overrides dxgkrnls power view of linked adapters.

        dprintf("Memory: %lu, MemoryBandwidth: %lu, PCIEBandwidth: %lu, FanRPM: %lu, Power: %lu, Temperature: %lu\n",
            adapterPerfData.MemoryFrequency,
            adapterPerfData.MemoryBandwidth,
            adapterPerfData.PCIEBandwidth,
            adapterPerfData.FanRPM,
            adapterPerfData.Power * 100 / 1000,
            adapterPerfData.Temperature * 100 / 1000
            );
    }
}

VOID EtpGpuDetailsEnumAdapters(
    _In_ HWND ListViewHandle
    )
{
    INT gpuAdapterGroupIndex;
    PETP_GPU_ADAPTER gpuAdapter;
    D3DKMT_OPENADAPTERFROMDEVICENAME openAdapterFromDeviceName;

    for (ULONG i = 0; i < EtpGpuAdapterList->Count; i++)
    {
        gpuAdapter = EtpGpuAdapterList->Items[i];

        memset(&openAdapterFromDeviceName, 0, sizeof(D3DKMT_OPENADAPTERFROMLUID));
        openAdapterFromDeviceName.DeviceName = PhGetString(gpuAdapter->DeviceInterface);

        if (!NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName(&openAdapterFromDeviceName)))
            continue;

        if ((gpuAdapterGroupIndex = PhAddListViewGroup(ListViewHandle, i, PhGetString(gpuAdapter->Description))) == MAXINT)
        {
            EtCloseAdapterHandle(openAdapterFromDeviceName.AdapterHandle);
            continue;
        }

        EtpGpuDetailsAddListViewItemGroups(ListViewHandle, gpuAdapterGroupIndex);

        EtpQueryAdapterDeviceProperties(openAdapterFromDeviceName.DeviceName, ListViewHandle);
        EtpQueryAdapterRegistryInfo(openAdapterFromDeviceName.AdapterHandle, ListViewHandle);
        EtpQueryAdapterDriverModel(openAdapterFromDeviceName.AdapterHandle, ListViewHandle);
        //EtpQueryAdapterDriverVersion(openAdapterFromDeviceName.AdapterHandle, ListViewHandle);
        EtpQueryAdapterDeviceIds(openAdapterFromDeviceName.AdapterHandle, ListViewHandle);
        EtpQueryAdapterPerfInfo(openAdapterFromDeviceName.AdapterHandle, ListViewHandle);

        EtCloseAdapterHandle(openAdapterFromDeviceName.AdapterHandle);
    }
}

INT_PTR CALLBACK EtpGpuDetailsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_LAYOUT_MANAGER LayoutManager;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND listViewHandle = GetDlgItem(hwndDlg, IDC_GPULIST);

            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER)));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER)));

            PhSetListViewStyle(listViewHandle, FALSE, TRUE);
            PhSetControlTheme(listViewHandle, L"explorer");
            PhAddListViewColumn(listViewHandle, 0, 0, 0, LVCFMT_LEFT, 290, L"Property");
            PhAddListViewColumn(listViewHandle, 1, 1, 1, LVCFMT_LEFT, 130, L"Value");
            PhSetExtendedListView(listViewHandle);
            ListView_EnableGroupView(listViewHandle, TRUE);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_GPULIST), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            // Note: This dialog must be centered after all other graphs and controls have been added.
            //if (PhGetIntegerPairSetting(SETTING_NAME_GPU_NODES_WINDOW_POSITION).X != 0)
            //    PhLoadWindowPlacementFromSetting(SETTING_NAME_GPU_NODES_WINDOW_POSITION, SETTING_NAME_GPU_NODES_WINDOW_SIZE, hwndDlg);
            //else
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            EtpGpuDetailsEnumAdapters(listViewHandle);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    }

    return FALSE;
}

VOID EtShowGpuDetailsDialog(
    _In_ HWND ParentWindowHandle
    )
{
    DialogBox(
        PluginInstance->DllBase, 
        MAKEINTRESOURCE(IDD_SYSINFO_GPUDETAILS),
        ParentWindowHandle,
        EtpGpuDetailsDlgProc
        );
}

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

typedef struct _COMMON_PAGE_CONTEXT
{
    PPH_STRING DiskName;
    DV_DISK_ID DiskId;
    ULONG DiskIndex;

    HANDLE DeviceHandle;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

typedef struct _DV_DISK_PAGE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;

    PH_LAYOUT_MANAGER LayoutManager;

    PCOMMON_PAGE_CONTEXT PageContext;
} DV_DISK_PAGE_CONTEXT, *PDV_DISK_PAGE_CONTEXT;

VOID DiskDriveQueryFileSystem(
    _Inout_ PDV_DISK_PAGE_CONTEXT Context
    )
{
    PPH_LIST deviceMountHandles;
    
    deviceMountHandles = DiskDriveQueryMountPointHandles(Context->PageContext->DiskIndex);

    for (ULONG i = 0; i < deviceMountHandles->Count; i++)
    {
        USHORT fsInfoType;
        PVOID fsInfoBuffer;
        PDISK_HANDLE_ENTRY diskEntry;

        diskEntry = deviceMountHandles->Items[i];

        if (DiskDriveQueryFileSystemInfo(diskEntry->DeviceHandle, &fsInfoType, &fsInfoBuffer))
        {
            switch (fsInfoType)
            {
            case FILESYSTEM_STATISTICS_TYPE_NTFS:
            case FILESYSTEM_STATISTICS_TYPE_REFS:
                {
                    PNTFS_FILESYSTEM_STATISTICS buffer = fsInfoBuffer;

#define LV_SHOW_VALUE(ListViewHandle, Name, Struct, Value) \
{ INT lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, ##Name, NULL); \
PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, PhaFormatString(L"%lu", buffer->Struct##.Value##)->Buffer); }

#define LV_SHOW_BYTE_VALUE(ListViewHandle, Name, Struct, Value) \
{ INT lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, ##Name, NULL); \
PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, PhaFormatSize(buffer->Struct##.Value##, -1)->Buffer); }


                    LV_SHOW_VALUE(Context->ListViewHandle, L"File reads", FileSystemStatistics, UserFileReads);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"File writes", FileSystemStatistics, UserFileWrites);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Disk reads", FileSystemStatistics, UserDiskReads);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Disk writes", FileSystemStatistics, UserDiskWrites);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"File read Bytes", FileSystemStatistics, UserFileReadBytes);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"File write Bytes", FileSystemStatistics, UserFileWriteBytes);


                    LV_SHOW_VALUE(Context->ListViewHandle, L"MetaData reads", FileSystemStatistics, MetaDataReads);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MetaData writes", FileSystemStatistics, MetaDataWrites);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MetaData disk reads", FileSystemStatistics, MetaDataDiskReads);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MetaData disk writes", FileSystemStatistics, MetaDataDiskWrites);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"MetaData read bytes", FileSystemStatistics, MetaDataReadBytes);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"MetaData write bytes", FileSystemStatistics, MetaDataWriteBytes);

                    // NTFS
                    LV_SHOW_VALUE(Context->ListViewHandle, L"LogFileFullExceptions", NtfsStatistics, LogFileFullExceptions);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"OtherExceptions", NtfsStatistics, OtherExceptions);

                    // Other meta data io's
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftReads", NtfsStatistics, MftReads);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftWrites", NtfsStatistics, MftWrites);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"MftReadBytes", NtfsStatistics, MftReadBytes);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"MftWriteBytes", NtfsStatistics, MftWriteBytes);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftWritesUserLevel-Write", NtfsStatistics, MftWritesUserLevel.Write);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftWritesUserLevel-Create", NtfsStatistics, MftWritesUserLevel.Create);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftWritesUserLevel-SetInfo", NtfsStatistics, MftWritesUserLevel.SetInfo);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftWritesUserLevel-Flush", NtfsStatistics, MftWritesUserLevel.Flush);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftWritesFlushForLogFileFull", NtfsStatistics, MftWritesFlushForLogFileFull);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftWritesLazyWriter", NtfsStatistics, MftWritesLazyWriter);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftWritesUserRequest", NtfsStatistics, MftWritesUserRequest);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Mft2Writes", NtfsStatistics, Mft2Writes);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"Mft2WriteBytes", NtfsStatistics, Mft2WriteBytes);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"Mft2WritesUserLevel-Write", NtfsStatistics, Mft2WritesUserLevel.Write);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Mft2WritesUserLevel-Create", NtfsStatistics, Mft2WritesUserLevel.Create);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Mft2WritesUserLevel-SetInfo", NtfsStatistics, Mft2WritesUserLevel.SetInfo);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Mft2WritesUserLevel-Flush", NtfsStatistics, Mft2WritesUserLevel.Flush);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"Mft2WritesFlushForLogFileFull", NtfsStatistics, Mft2WritesFlushForLogFileFull);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Mft2WritesLazyWriter", NtfsStatistics, Mft2WritesLazyWriter);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Mft2WritesUserRequest", NtfsStatistics, Mft2WritesUserRequest);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"RootIndexReads", NtfsStatistics, RootIndexReads);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"RootIndexWrites", NtfsStatistics, RootIndexWrites);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"RootIndexReadBytes", NtfsStatistics, RootIndexReadBytes);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"RootIndexWriteBytes", NtfsStatistics, RootIndexWriteBytes);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"BitmapReads", NtfsStatistics, BitmapReads);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"BitmapWrites", NtfsStatistics, BitmapWrites);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"BitmapReadBytes", NtfsStatistics, BitmapReadBytes);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"BitmapWriteBytes", NtfsStatistics, BitmapWriteBytes);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"BitmapWritesFlushForLogFileFull", NtfsStatistics, BitmapWritesFlushForLogFileFull);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"BitmapWritesLazyWriter", NtfsStatistics, BitmapWritesLazyWriter);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"BitmapWritesUserRequest", NtfsStatistics, BitmapWritesUserRequest);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"BitmapWritesUserRequest-Write", NtfsStatistics, BitmapWritesUserLevel.Write);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"BitmapWritesUserRequest-Create", NtfsStatistics, BitmapWritesUserLevel.Create);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"BitmapWritesUserRequest-SetInfo", NtfsStatistics, BitmapWritesUserLevel.SetInfo);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftBitmapReads", NtfsStatistics, MftBitmapReads);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftBitmapWrites", NtfsStatistics, MftBitmapWrites);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"MftBitmapReadBytes", NtfsStatistics, MftBitmapReadBytes);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"MftBitmapWriteBytes", NtfsStatistics, MftBitmapWriteBytes);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftBitmapWritesFlushForLogFileFull", NtfsStatistics, MftBitmapWritesFlushForLogFileFull);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftBitmapWritesLazyWriter", NtfsStatistics, MftBitmapWritesLazyWriter);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftBitmapWritesUserRequest", NtfsStatistics, MftBitmapWritesUserRequest);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftBitmapWritesUserLevel-Write", NtfsStatistics, MftBitmapWritesUserLevel.Write);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftBitmapWritesUserLevel-Create", NtfsStatistics, MftBitmapWritesUserLevel.Create);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftBitmapWritesUserLevel-SetInfo", NtfsStatistics, MftBitmapWritesUserLevel.SetInfo);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"MftBitmapWritesUserLevel-Flush", NtfsStatistics, MftBitmapWritesUserLevel.Flush);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"UserIndexReads", NtfsStatistics, UserIndexReads);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"UserIndexWrites", NtfsStatistics, UserIndexWrites);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"UserIndexReadBytes", NtfsStatistics, UserIndexReadBytes);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"UserIndexWriteBytes", NtfsStatistics, UserIndexWriteBytes);

                    // Additions for NT 5.0
                    LV_SHOW_VALUE(Context->ListViewHandle, L"LogFileReads", NtfsStatistics, LogFileReads);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"LogFileWrites", NtfsStatistics, LogFileWrites);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"LogFileReadBytes", NtfsStatistics, LogFileReadBytes);
                    LV_SHOW_BYTE_VALUE(Context->ListViewHandle, L"LogFileWriteBytes", NtfsStatistics, LogFileWriteBytes);

                    LV_SHOW_VALUE(Context->ListViewHandle, L"Allocate-Calls", NtfsStatistics, Allocate.Calls);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Allocate-Clusters", NtfsStatistics, Allocate.Clusters);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Allocate-Hints", NtfsStatistics, Allocate.Hints);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Allocate-RunsReturned", NtfsStatistics, Allocate.RunsReturned);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Allocate-HintsHonored", NtfsStatistics, Allocate.HintsHonored);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Allocate-HintsClusters", NtfsStatistics, Allocate.HintsClusters);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Allocate-Cache", NtfsStatistics, Allocate.Cache);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Allocate-CacheClusters", NtfsStatistics, Allocate.CacheClusters);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Allocate-CacheMiss", NtfsStatistics, Allocate.CacheMiss);
                    LV_SHOW_VALUE(Context->ListViewHandle, L"Allocate-CacheMissClusters", NtfsStatistics, Allocate.CacheMissClusters);

                    //  Additions for Windows 8.1
                    LV_SHOW_VALUE(Context->ListViewHandle, L"DiskResourcesExhausted", NtfsStatistics, DiskResourcesExhausted);
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
    }
}

VOID DiskDriveQuerySmart(
    _Inout_ PDV_DISK_PAGE_CONTEXT Context
    )
{
    PPH_LIST attributes;

    if (DiskDriveQueryImminentFailure(Context->PageContext->DeviceHandle, &attributes))
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
            }

            PhFree(attribute);
        }

        PhDereferenceObject(attributes);
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

INT_PTR CALLBACK DiskDriveSmartDetailsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_PAGE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;

        context = PhAllocate(sizeof(DV_DISK_PAGE_CONTEXT));
        memset(context, 0, sizeof(DV_DISK_PAGE_CONTEXT));

        context->PageContext = (PCOMMON_PAGE_CONTEXT)propSheetPage->lParam;

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PDV_DISK_PAGE_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DETAILS_LIST);

            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 240, L"Property");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 50, L"Value");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"Best");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Raw");
            PhSetExtendedListView(context->ListViewHandle);
            //ExtendedListView_SetItemColorFunction(context->ListViewHandle, PhpColorItemColorFunction);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DESCRIPTION), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_EDIT1), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            DiskDriveQuerySmart(context);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == LVN_ITEMCHANGED)
            {
                PWSTR description;

                if (ListView_GetSelectedCount(context->ListViewHandle) == 1)
                    description = SmartAttributeGetDescription((SMART_ATTRIBUTE_ID)PhGetSelectedListViewItemParam(context->ListViewHandle));
                else
                    description = L"";

                SetDlgItemText(hwndDlg, IDC_EDIT1, description);
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK DiskDriveFileSystemDetailsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_PAGE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;

        context = PhAllocate(sizeof(DV_DISK_PAGE_CONTEXT));
        memset(context, 0, sizeof(DV_DISK_PAGE_CONTEXT));

        context->PageContext = (PCOMMON_PAGE_CONTEXT)propSheetPage->lParam;

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PDV_DISK_PAGE_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_DETAILS_LIST);

            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 300, L"Property");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Value");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            DiskDriveQueryFileSystem(context);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
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

VOID ShowDiskDriveDetailsDialog(
    _In_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[2];

    pageContext = PhAllocate(sizeof(COMMON_PAGE_CONTEXT));
    memset(pageContext, 0, sizeof(COMMON_PAGE_CONTEXT));
    pageContext->DiskIndex = Context->DiskEntry->DiskIndex;
    PhSetReference(&pageContext->DiskName, Context->DiskEntry->DiskName);
    CopyDiskId(&pageContext->DiskId, &Context->DiskEntry->Id);


    HANDLE deviceHandle;
    if (NT_SUCCESS(DiskDriveCreateHandle(&deviceHandle, Context->DiskEntry->Id.DevicePath)))
    {
        pageContext->DeviceHandle = deviceHandle;
    }


    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = (HWND)Context->WindowHandle;
    propSheetHeader.pszCaption = L"Disk Drive";
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // General
    //memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    //propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    //propSheetPage.hInstance = PluginInstance->DllBase;
    //propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DISKDRIVE_DETAILS_GENERAL);
    //propSheetPage.pfnDlgProc = DiskDriveDetailsDlgProc;
    //pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
   
    // FileSystem
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DISKDRIVE_DETAILS_FILESYSTEM);
    propSheetPage.pfnDlgProc = DiskDriveFileSystemDetailsDlgProc;
    propSheetPage.lParam = (LPARAM)pageContext;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // SMART
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DISKDRIVE_DETAILS_SMART);
    propSheetPage.pfnDlgProc = DiskDriveSmartDetailsDlgProc;
    propSheetPage.lParam = (LPARAM)pageContext;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    PhModalPropertySheet(&propSheetHeader);

    FreeDiskDriveDetailsContext(pageContext);
}
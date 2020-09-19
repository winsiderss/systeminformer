/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2017-2020 dmex
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

#include <peview.h>

static PWSTR PvpGetResourceTypeString(
    _In_ ULONG_PTR Type
    )
{
    switch (Type)
    {
    case RT_CURSOR:
        return L"RT_CURSOR";
    case RT_BITMAP:
        return L"RT_BITMAP";
    case RT_ICON:
        return L"RT_ICON";
    case RT_MENU:
        return L"RT_MENU";
    case RT_DIALOG:
        return L"RT_DIALOG";
    case RT_STRING:
        return L"RT_STRING";
    case RT_FONTDIR:
        return L"RT_FONTDIR";
    case RT_FONT:
        return L"RT_FONT";
    case RT_ACCELERATOR:
        return L"RT_ACCELERATOR";
    case RT_RCDATA:
        return L"RT_RCDATA";
    case RT_MESSAGETABLE:
        return L"RT_MESSAGETABLE";
    case RT_GROUP_CURSOR:
        return L"RT_GROUP_CURSOR";
    case RT_GROUP_ICON:
        return L"RT_GROUP_ICON";
    case RT_VERSION:
        return L"RT_VERSION";
    case RT_ANICURSOR:
        return L"RT_ANICURSOR";
    case RT_MANIFEST:
        return L"RT_MANIFEST";
    }

    return PhaFormatString(L"%lu", (ULONG)Type)->Buffer;
}

PPH_STRING PvpPeResourceDumpFileName(
    _In_ HWND ParentWindow
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Resouce data (*.data)", L"*.data" },
        { L"All files (*.*)", L"*.*" }
    };
    PPH_STRING fileName = NULL;
    PVOID fileDialog;

    if (fileDialog = PhCreateSaveFileDialog())
    {
        PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));
        //PhSetFileDialogFileName(fileDialog, L"file.data");

        if (PhShowFileDialog(ParentWindow, fileDialog))
        {
            fileName = PhGetFileDialogFileName(fileDialog);
        }

        PhFreeFileDialog(fileDialog);
    }

    return fileName;
}

VOID PvpPeResourceSaveToFile(
    _In_ HWND WindowHandle,
    _In_ PVOID ListViewParam
    )
{
    PH_MAPPED_IMAGE_RESOURCES resources;
    PH_IMAGE_RESOURCE_ENTRY entry;
    PPH_STRING fileName;

    if (!(fileName = PvpPeResourceDumpFileName(WindowHandle)))
        return;

    if (NT_SUCCESS(PhGetMappedImageResources(&resources, &PvMappedImage)))
    {
        for (ULONG i = 0; i < resources.NumberOfEntries; i++)
        {
            if (i != PtrToUlong(ListViewParam))
                continue;

            entry = resources.ResourceEntries[i];

            if (entry.Data && entry.Size)
            {
                NTSTATUS status;
                HANDLE fileHandle;

                status = PhCreateFileWin32(
                    &fileHandle,
                    PhGetString(fileName),
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OVERWRITE_IF,
                    FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                    );

                if (NT_SUCCESS(status))
                {
                    IO_STATUS_BLOCK isb;

                    __try
                    {
                        status = NtWriteFile(
                            fileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &isb,
                            entry.Data,
                            entry.Size,
                            NULL,
                            NULL
                            );
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();    
                    }

                    NtClose(fileHandle);
                }

                if (!NT_SUCCESS(status))
                {
                    PhShowStatus(WindowHandle, L"Unable to save resource.", status, 0);
                }
            }
        }

        PhFree(resources.ResourceEntries);
    }

    PhDereferenceObject(fileName);
}

typedef enum _PVE_RESOURCES_COLUMN_INDEX
{
    PVE_RESOURCES_COLUMN_INDEX_COUNT,
    PVE_RESOURCES_COLUMN_INDEX_TYPE,
    PVE_RESOURCES_COLUMN_INDEX_NAME,
    PVE_RESOURCES_COLUMN_INDEX_SIZE,
    PVE_RESOURCES_COLUMN_INDEX_LCID,
    PVE_RESOURCES_COLUMN_INDEX_HASH
} PVE_RESOURCES_COLUMN_INDEX;

INT_PTR CALLBACK PvpPeResourcesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;
            PH_MAPPED_IMAGE_RESOURCES resources;
            PH_IMAGE_RESOURCE_ENTRY entry;
            ULONG count = 0;
            ULONG i;
            INT lvItemIndex;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Type");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Name");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Size");
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Language");
            PhAddListViewColumn(lvHandle, 5, 5, 5, LVCFMT_LEFT, 100, L"Hash");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageResourcesListViewColumns", lvHandle);

            if (NT_SUCCESS(PhGetMappedImageResources(&resources, &PvMappedImage)))
            {
                for (i = 0; i < resources.NumberOfEntries; i++)
                {
                    PPH_STRING string;
                    WCHAR number[PH_INT32_STR_LEN_1];

                    entry = resources.ResourceEntries[i];

                    PhPrintUInt32(number, ++count);
                    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, number, UlongToPtr(i));

                    if (IS_INTRESOURCE(entry.Type))
                    {
                        PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_TYPE, PvpGetResourceTypeString(entry.Type));
                    }
                    else
                    {
                        PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Type;

                        string = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
                        PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_TYPE, string->Buffer);
                        PhDereferenceObject(string);
                    }

                    if (IS_INTRESOURCE(entry.Name))
                    {
                        PhPrintUInt32(number, (ULONG)entry.Name);
                        PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_NAME, number);
                    }
                    else
                    {
                        PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Name;

                        string = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
                        PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_NAME, string->Buffer);
                        PhDereferenceObject(string);
                    }

                    if (IS_INTRESOURCE(entry.Language))
                    {
                        if ((ULONG)entry.Language)
                        {
#if (PHNT_VERSION >= PHNT_WIN7)
                            UNICODE_STRING localeNameUs;
                            WCHAR localeName[LOCALE_NAME_MAX_LENGTH] = { UNICODE_NULL };

                            RtlInitEmptyUnicodeString(&localeNameUs, localeName, sizeof(localeName));

                            if (NT_SUCCESS(RtlLcidToLocaleName((ULONG)entry.Language, &localeNameUs, 0, FALSE)))
                            {
                                PhPrintUInt32(number, (ULONG)entry.Language);
                                PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_LCID, PhaFormatString(L"%s (%s)", number, localeName)->Buffer);
                            }
                            else
                            {
                                PhPrintUInt32(number, (ULONG)entry.Language);
                                PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_LCID, number);
                            }
#else
                            PhPrintUInt32(number, (ULONG)entry.Language);
                            PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_LCID, number);
#endif
                        }
                        else // LOCALE_NEUTRAL
                        {
                            PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_LCID, L"Neutral");
                        }
                    }
                    else
                    {
                        PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Language;

                        string = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
                        PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_LCID, string->Buffer);
                        PhDereferenceObject(string);
                    }

                    PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_SIZE, PhaFormatSize(entry.Size, ULONG_MAX)->Buffer);

                    if (entry.Data && entry.Size)
                    {
                        __try
                        {
                            PH_HASH_CONTEXT hashContext;
                            PPH_STRING hashString;
                            UCHAR hash[32];

                            PhInitializeHash(&hashContext, Md5HashAlgorithm);
                            PhUpdateHash(&hashContext, entry.Data, entry.Size);

                            if (PhFinalHash(&hashContext, hash, 16, NULL))
                            {
                                if (hashString = PhBufferToHexString(hash, 16))
                                {
                                    PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_HASH, hashString->Buffer);
                                    PhDereferenceObject(hashString);
                                }
                            }
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            PPH_STRING message;

                            //message = PH_AUTO(PhGetNtMessage(GetExceptionCode()));
                            message = PH_AUTO(PhGetWin32Message(RtlNtStatusToDosError(GetExceptionCode()))); // WIN32_FROM_NTSTATUS

                            PhSetListViewSubItem(lvHandle, lvItemIndex, PVE_RESOURCES_COLUMN_INDEX_HASH, PhGetStringOrEmpty(message));
                        }
                    }
                }

                PhFree(resources.ResourceEntries);
            }

            ExtendedListView_SortItems(lvHandle);
            
            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageResourcesListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST), dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == GetDlgItem(hwndDlg, IDC_LIST))
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PvGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams((HWND)wParam, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Save resource", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"&Copy", NULL, NULL), ULONG_MAX);
                    PvInsertCopyListViewEMenuItem(menu, USHRT_MAX, (HWND)wParam);

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
                        if (!PvHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case 1:
                                PvpPeResourceSaveToFile(hwndDlg, listviewItems[0]);
                                break;
                            case USHRT_MAX:
                                PvCopyListView((HWND)wParam);
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

/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2017 dmex
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

PWSTR PvpGetResourceTypeString(ULONG_PTR Type)
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

    return L"ERROR";
}

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
            NTSTATUS status;
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
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Name");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 150, L"Type");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Language");
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Size");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageResourcesListViewColumns", lvHandle);

            status = PhGetMappedImageResources(&resources, &PvMappedImage);

            if (NT_SUCCESS(status))
            {
                for (i = 0; i < resources.NumberOfEntries; i++)
                {
                    PPH_STRING string;
                    WCHAR number[PH_INT32_STR_LEN_1];

                    entry = resources.ResourceEntries[i];

                    PhPrintUInt64(number, ++count);
                    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, number, NULL);

                    if (IS_INTRESOURCE(entry.Name))
                    {
                        PhPrintUInt32(number, (ULONG)entry.Name);
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 1, number);
                    }
                    else
                    {
                        PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Name;

                        string = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 1, string->Buffer);
                        PhDereferenceObject(string);
                    }

                    if (IS_INTRESOURCE(entry.Type))
                    {
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PvpGetResourceTypeString(entry.Type));
                    }
                    else
                    {
                        PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Type;

                        string = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 2, string->Buffer);
                        PhDereferenceObject(string);
                    }

                    if (IS_INTRESOURCE(entry.Language))
                    {
                        WCHAR localeName[LOCALE_NAME_MAX_LENGTH];

                        PhPrintUInt32(number, (ULONG)entry.Language);

                        if (LCIDToLocaleName((ULONG)entry.Language, localeName, LOCALE_NAME_MAX_LENGTH, LOCALE_ALLOW_NEUTRAL_NAMES))
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 3, PhaFormatString(L"%s (%s)", number, localeName)->Buffer);
                        else
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 3, number);
                    }
                    else
                    {
                        PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Language;

                        string = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 3, string->Buffer);
                        PhDereferenceObject(string);
                    }

                    PhSetListViewSubItem(lvHandle, lvItemIndex, 4, PhaFormatSize(entry.Size, -1)->Buffer);
                }

                if (resources.ResourceEntries)
                    PhFree(resources.ResourceEntries);
            }
            else
            {
                PhShowStatus(hwndDlg, L"Unable to enumerate module resources.", status, 0);
            }

            ExtendedListView_SortItems(lvHandle);
            
            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
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

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST),
                    dialogItem, PH_ANCHOR_ALL);

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
    }

    return FALSE;
}

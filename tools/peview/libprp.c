/*
 * Process Hacker - 
 *   PE viewer
 * 
 * Copyright (C) 2010 wj32
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

INT_PTR CALLBACK PvpLibExportsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

PH_MAPPED_ARCHIVE PvMappedArchive;

VOID PvLibProperties()
{
    NTSTATUS status;
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[1];

    status = PhLoadMappedArchive(PvFileName->Buffer, NULL, TRUE, &PvMappedArchive);

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(NULL, L"Unable to load the archive file", status, 0);
        return;
    }

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hwndParent = NULL;
    propSheetHeader.pszCaption = PvFileName->Buffer;
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // Exports page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_LIBEXPORTS);
    propSheetPage.pfnDlgProc = PvpLibExportsDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    PropertySheet(&propSheetHeader);

    PhUnloadMappedArchive(&PvMappedArchive);
}

INT_PTR CALLBACK PvpLibExportsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG fallbackColumns[] = { 0, 1, 2, 3 };
            HWND lvHandle;
            PH_MAPPED_ARCHIVE_MEMBER member;
            PH_MAPPED_ARCHIVE_IMPORT_ENTRY importEntry;

            PhCenterWindow(GetParent(hwndDlg), NULL);

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 60, L"DLL");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 40, L"Ordinal/Hint");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 40, L"Type");
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 60, L"Name Type");
            PhSetExtendedListView(lvHandle);
            ExtendedListView_AddFallbackColumns(lvHandle, 4, fallbackColumns);

            member = *PvMappedArchive.LastStandardMember;

            while (NT_SUCCESS(PhGetNextMappedArchiveMember(&member, &member)))
            {
                if (NT_SUCCESS(PhGetMappedArchiveImportEntry(&member, &importEntry)))
                {
                    INT lvItemIndex;
                    PPH_STRING name;
                    WCHAR number[PH_INT32_STR_LEN_1];
                    PWSTR type;

                    name = PhCreateStringFromAnsi(importEntry.DllName);
                    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, name->Buffer, NULL);
                    PhDereferenceObject(name);

                    name = PhCreateStringFromAnsi(importEntry.Name);
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, name->Buffer);
                    PhDereferenceObject(name);

                    // Ordinal is unioned with NameHint, so this works both ways.
                    PhPrintUInt32(number, importEntry.Ordinal);
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 2, number);

                    switch (importEntry.Type)
                    {
                    case IMPORT_OBJECT_CODE:
                        type = L"Code";
                        break;
                    case IMPORT_OBJECT_DATA:
                        type = L"Data";
                        break;
                    case IMPORT_OBJECT_CONST:
                        type = L"Const";
                        break;
                    default:
                        type = L"Unknown";
                        break;
                    }

                    PhSetListViewSubItem(lvHandle, lvItemIndex, 3, type);

                    switch (importEntry.NameType)
                    {
                    case IMPORT_OBJECT_ORDINAL:
                        type = L"Ordinal";
                        break;
                    case IMPORT_OBJECT_NAME:
                        type = L"Name";
                        break;
                    case IMPORT_OBJECT_NAME_NO_PREFIX:
                        type = L"Name, No Prefix";
                        break;
                    case IMPORT_OBJECT_NAME_UNDECORATE:
                        type = L"Name, Undecorate";
                        break;
                    default:
                        type = L"Unknown";
                        break;
                    }

                    PhSetListViewSubItem(lvHandle, lvItemIndex, 4, type);
                }
            }

            ExtendedListView_SortItems(lvHandle);
        }
        break;
    }

    return FALSE;
}

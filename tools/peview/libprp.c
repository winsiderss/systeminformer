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
#include <mapimg.h>
#include <uxtheme.h>

INT_PTR CALLBACK PvpLibExportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

PH_MAPPED_ARCHIVE PvMappedArchive;

VOID PvLibProperties(
    VOID
    )
{
    NTSTATUS status;
    PPV_PROPCONTEXT propContext;

    status = PhLoadMappedArchive(PvFileName->Buffer, NULL, TRUE, &PvMappedArchive);

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(NULL, L"Unable to load the archive file", status, 0);
        return;
    }

    if (propContext = PvCreatePropContext(PvFileName))
    {
        PPV_PROPPAGECONTEXT newPage;

        // Lib page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_LIBEXPORTS),
            PvpLibExportsDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        PhModalPropertySheet(&propContext->PropSheetHeader);

        PhDereferenceObject(propContext);
    }

    PhUnloadMappedArchive(&PvMappedArchive);
}

INT_PTR CALLBACK PvpLibExportsDlgProc(
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
            ULONG fallbackColumns[] = { 0, 1, 2, 3 };
            HWND lvHandle;
            PH_MAPPED_ARCHIVE_MEMBER member;
            PH_MAPPED_ARCHIVE_IMPORT_ENTRY importEntry;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 60, L"DLL");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 40, L"Ordinal/Hint");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 40, L"Type");
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 60, L"Name type");
            PhSetExtendedListView(lvHandle);
            ExtendedListView_AddFallbackColumns(lvHandle, 4, fallbackColumns);
            PhLoadListViewColumnsFromSetting(L"LibListViewColumns", lvHandle);

            member = *PvMappedArchive.LastStandardMember;

            while (NT_SUCCESS(PhGetNextMappedArchiveMember(&member, &member)))
            {
                if (NT_SUCCESS(PhGetMappedArchiveImportEntry(&member, &importEntry)))
                {
                    INT lvItemIndex;
                    PPH_STRING name;
                    WCHAR number[PH_INT32_STR_LEN_1];
                    PWSTR type;

                    name = PhZeroExtendToUtf16(importEntry.DllName);
                    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, name->Buffer, NULL);
                    PhDereferenceObject(name);

                    name = PhZeroExtendToUtf16(importEntry.Name);
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
                        type = L"Name, no prefix";
                        break;
                    case IMPORT_OBJECT_NAME_UNDECORATE:
                        type = L"Name, undecorate";
                        break;
                    default:
                        type = L"Unknown";
                        break;
                    }

                    PhSetListViewSubItem(lvHandle, lvItemIndex, 4, type);
                }
            }

            ExtendedListView_SortItems(lvHandle);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"LibListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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

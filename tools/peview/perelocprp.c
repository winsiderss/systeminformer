/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2021 dmex
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

VOID PvEnumerateRelocationEntries(
    _In_ HWND ListViewHandle
    )
{
    ULONG count = 0;
    PH_MAPPED_IMAGE_RELOC relocations;

    if (NT_SUCCESS(PhGetMappedImageRelocations(&PvMappedImage, &relocations)))
    {
        for (ULONG i = 0; i < relocations.NumberOfEntries; i++)
        {
            PPH_IMAGE_RELOC_ENTRY entry = PTR_ADD_OFFSET(relocations.RelocationEntries, i * sizeof(PH_IMAGE_RELOC_ENTRY));
            INT lvItemIndex;
            WCHAR value[PH_INT64_STR_LEN_1];

            PhPrintUInt64(value, ++count);
            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);
            PhPrintPointer(value, UlongToPtr(entry->BlockRva));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);
            PhPrintPointer(value, UlongToPtr(entry->Offset));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, value);

            switch (entry->Type)
            {
            case IMAGE_REL_BASED_ABSOLUTE:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, L"ABS");
                break;
            case IMAGE_REL_BASED_HIGH:
                {
                    USHORT highValue = USHRT_MAX;

                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, L"HIGH");

                    if (entry->Value)
                    {
                        __try
                        {
                            highValue = *(PUSHORT)entry->Value;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            NOTHING;
                        }
                    }

                    if (highValue != USHRT_MAX)
                    {
                        PhPrintPointer(value, (PVOID)highValue);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, value);
                    }
                }
                break;
            case IMAGE_REL_BASED_LOW:
                {
                    USHORT lowValue = USHRT_MAX;

                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, L"LOW");

                    if (entry->Value)
                    {
                        __try
                        {
                            lowValue = *(PUSHORT)entry->Value;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            NOTHING;
                        }
                    }

                    if (lowValue != USHRT_MAX)
                    {
                        PhPrintPointer(value, (PVOID)lowValue);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, value);
                    }
                }
                break;
            case IMAGE_REL_BASED_HIGHLOW:
                {
                    ULONG highlowValue = ULONG_MAX;

                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, L"HIGHLOW");

                    if (entry->Value)
                    {
                        __try
                        {
                            highlowValue = *(PULONG)entry->Value;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            NOTHING;
                        }
                    }

                    if (highlowValue && highlowValue != ULONG_MAX)
                    {
                        PhPrintPointer(value, UlongToPtr(highlowValue));
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, value);
                    }
                }
                break;
            case IMAGE_REL_BASED_DIR64:
                {
                    ULONGLONG dirValue = ULLONG_MAX;

                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, L"DIR64");

                    if (entry->Value)
                    {
                        __try
                        {
                            dirValue = *(PULONGLONG)entry->Value;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            NOTHING;
                        }
                    }

                    if (dirValue && dirValue != ULLONG_MAX)
                    {
                        PhPrintPointer(value, (PVOID)dirValue);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, value);
                    }
                }
                break;
            }

            if (entry->BlockRva && entry->Offset)
            {
                PIMAGE_SECTION_HEADER directorySection;

                directorySection = PhMappedImageRvaToSection(
                    &PvMappedImage,
                    PtrToUlong(PTR_ADD_OFFSET(entry->BlockRva, entry->Offset))
                    );

                if (directorySection)
                {
                    WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

                    if (PhGetMappedImageSectionName(
                        directorySection,
                        sectionName,
                        RTL_NUMBER_OF(sectionName),
                        NULL
                        ))
                    {
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 5, sectionName);
                    }
                }
            }
        }
    }
}

INT_PTR CALLBACK PvpPeRelocationDlgProc(
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

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"RVA");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Offset");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 150, L"Value");
            PhAddListViewColumn(lvHandle, 5, 5, 5, LVCFMT_LEFT, 100, L"Section");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageRelocationsListViewColumns", lvHandle);

            PvEnumerateRelocationEntries(lvHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageRelocationsListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    }

    return FALSE;
}

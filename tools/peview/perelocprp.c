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
            ULONGLONG dirValue = ULLONG_MAX;
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
                {
                    ULONG imageMachine;

                    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                        imageMachine = PvMappedImage.NtHeaders32->FileHeader.Machine;
                    else
                        imageMachine = PvMappedImage.NtHeaders->FileHeader.Machine;

                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, L"ABS");

                    if (entry->Value)
                    {
                        __try
                        {
                            if (imageMachine == IMAGE_FILE_MACHINE_I386)
                            {
                                dirValue = *(PULONG)entry->Value;
                            }
                            else if (imageMachine == IMAGE_FILE_MACHINE_AMD64)
                            {
                                dirValue = *(PULONGLONG)entry->Value;
                            }
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            NOTHING;
                        }
                    }

                    if (dirValue != ULLONG_MAX)
                    {
                        PhPrintPointer(value, (PVOID)dirValue);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, value);
                    }
                }
                break;
            case IMAGE_REL_BASED_HIGH:
                {
                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, L"HIGH");

                    if (entry->Value)
                    {
                        __try
                        {
                            dirValue = *(PUSHORT)entry->Value;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            NOTHING;
                        }
                    }

                    if (dirValue != ULLONG_MAX)
                    {
                        PhPrintPointer(value, (PVOID)dirValue);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, value);
                    }
                }
                break;
            case IMAGE_REL_BASED_LOW:
                {
                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, L"LOW");

                    if (entry->Value)
                    {
                        __try
                        {
                            dirValue = *(PUSHORT)entry->Value;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            NOTHING;
                        }
                    }

                    if (dirValue != ULLONG_MAX)
                    {
                        PhPrintPointer(value, (PVOID)dirValue);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, value);
                    }
                }
                break;
            case IMAGE_REL_BASED_HIGHLOW:
                {
                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, L"HIGHLOW");

                    if (entry->Value)
                    {
                        __try
                        {
                            dirValue = *(PULONG)entry->Value;
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
            case IMAGE_REL_BASED_DIR64:
                {
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

            if (entry->BlockRva)
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

            if (dirValue)
            {
                if (
                    (dirValue >= PvMappedImage.NtHeaders->OptionalHeader.ImageBase) &&
                    (dirValue < PvMappedImage.NtHeaders->OptionalHeader.ImageBase + PvMappedImage.NtHeaders->OptionalHeader.SizeOfImage)
                    )
                {
                    PPH_STRING symbol = PhGetSymbolFromAddress(
                        PvSymbolProvider,
                        (ULONG64)dirValue,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );

                    if (symbol)
                    {
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 6, symbol->Buffer);
                        PhDereferenceObject(symbol);
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
            PhAddListViewColumn(lvHandle, 6, 6, 6, LVCFMT_LEFT, 100, L"Symbol");
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

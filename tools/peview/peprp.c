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

INT_PTR CALLBACK PvpPeGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PvpPeImportsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PvpPeExportsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

PH_MAPPED_IMAGE PvMappedImage;

VOID PvPeProperties()
{
    NTSTATUS status;
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[3];

    status = PhLoadMappedImage(PvFileName->Buffer, NULL, TRUE, &PvMappedImage);

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(NULL, L"Unable to load the PE file", status, 0);
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

    // General page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PEGENERAL);
    propSheetPage.pfnDlgProc = PvpPeGeneralDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Imports page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PEIMPORTS);
    propSheetPage.pfnDlgProc = PvpPeImportsDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Exports page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PEEXPORTS);
    propSheetPage.pfnDlgProc = PvpPeExportsDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    PropertySheet(&propSheetHeader);

    PhUnloadMappedImage(&PvMappedImage);
}

INT_PTR CALLBACK PvpPeGeneralDlgProc(
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
            HWND lvHandle;
            ULONG i;
            PPH_STRING string;
            PWSTR type;
            WCHAR pointer[PH_PTR_STR_LEN_1];
            PH_STRING_BUILDER stringBuilder;

            PhCenterWindow(GetParent(hwndDlg), NULL);

            switch (PvMappedImage.NtHeaders->FileHeader.Machine)
            {
            case IMAGE_FILE_MACHINE_I386:
                type = L"i386";
                break;
            case IMAGE_FILE_MACHINE_AMD64:
                type = L"AMD64";
                break;
            case IMAGE_FILE_MACHINE_IA64:
                type = L"IA64";
                break;
            default:
                type = L"Unknown";
                break;
            }

            SetDlgItemText(hwndDlg, IDC_TARGETMACHINE, type);

            PhPrintPointer(pointer, (PVOID)PvMappedImage.NtHeaders->OptionalHeader.CheckSum);
            SetDlgItemText(hwndDlg, IDC_CHECKSUM, pointer);

            switch (PvMappedImage.NtHeaders->OptionalHeader.Subsystem)
            {
            case IMAGE_SUBSYSTEM_NATIVE:
                type = L"Native";
                break;
            case IMAGE_SUBSYSTEM_WINDOWS_GUI:
                type = L"Windows GUI";
                break;
            case IMAGE_SUBSYSTEM_WINDOWS_CUI:
                type = L"Windows CUI";
                break;
            case IMAGE_SUBSYSTEM_POSIX_CUI:
                type = L"POSIX CUI";
                break;
            case IMAGE_SUBSYSTEM_EFI_APPLICATION:
                type = L"EFI Application";
                break;
            case IMAGE_SUBSYSTEM_XBOX:
                type = L"Xbox";
                break;
            default:
                type = L"Unknown";
                break;
            }

            SetDlgItemText(hwndDlg, IDC_SUBSYSTEM, type);

            string = PhFormatString(
                L"%u.%u",
                PvMappedImage.NtHeaders->OptionalHeader.MajorSubsystemVersion,
                PvMappedImage.NtHeaders->OptionalHeader.MinorSubsystemVersion
                );
            SetDlgItemText(hwndDlg, IDC_SUBSYSTEMVERSION, string->Buffer);
            PhDereferenceObject(string);

            PhInitializeStringBuilder(&stringBuilder, 10);

            if (PvMappedImage.NtHeaders->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)
                PhAppendStringBuilder2(&stringBuilder, L"Executable, ");
            if (PvMappedImage.NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL)
                PhAppendStringBuilder2(&stringBuilder, L"DLL, ");
            if (PvMappedImage.NtHeaders->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)
                PhAppendStringBuilder2(&stringBuilder, L"Large address aware, ");
            if (PvMappedImage.NtHeaders->FileHeader.Characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP)
                PhAppendStringBuilder2(&stringBuilder, L"Removable run from swap, ");
            if (PvMappedImage.NtHeaders->FileHeader.Characteristics & IMAGE_FILE_NET_RUN_FROM_SWAP)
                PhAppendStringBuilder2(&stringBuilder, L"Net run from swap, ");
            if (PvMappedImage.NtHeaders->FileHeader.Characteristics & IMAGE_FILE_SYSTEM)
                PhAppendStringBuilder2(&stringBuilder, L"System, ");
            if (PvMappedImage.NtHeaders->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY)
                PhAppendStringBuilder2(&stringBuilder, L"Uni-processor only, ");

            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)
                PhAppendStringBuilder2(&stringBuilder, L"Dynamic base, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY)
                PhAppendStringBuilder2(&stringBuilder, L"Force integrity check, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NX_COMPAT)
                PhAppendStringBuilder2(&stringBuilder, L"NX compatible, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_SEH)
                PhAppendStringBuilder2(&stringBuilder, L"No SEH, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE)
                PhAppendStringBuilder2(&stringBuilder, L"Terminal server aware, ");

            if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 2, 2);

            SetDlgItemText(hwndDlg, IDC_CHARACTERISTICS, stringBuilder.String->Buffer);
            PhDeleteStringBuilder(&stringBuilder);

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 80, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"VA");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Size");

            for (i = 0; i < PvMappedImage.NumberOfSections; i++)
            {
                INT lvItemIndex;
                WCHAR sectionName[9];
                WCHAR pointer[PH_PTR_STR_LEN_1];

                if (PhCopyUnicodeStringZFromAnsi(PvMappedImage.Sections[i].Name,
                    IMAGE_SIZEOF_SHORT_NAME, sectionName, 9, NULL))
                {
                    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, sectionName, NULL);

                    PhPrintPointer(pointer, (PVOID)PvMappedImage.Sections[i].VirtualAddress);
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, pointer);

                    PhPrintPointer(pointer, (PVOID)PvMappedImage.Sections[i].SizeOfRawData);
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 2, pointer); 
                }
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PvpPeImportsDlgProc(
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
            ULONG fallbackColumns[] = { 0, 1, 2 };
            HWND lvHandle;
            PH_MAPPED_IMAGE_IMPORTS imports;
            PH_MAPPED_IMAGE_IMPORT_DLL importDll;
            PH_MAPPED_IMAGE_IMPORT_ENTRY importEntry;
            ULONG i;
            ULONG j;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"DLL");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"Hint");
            PhSetExtendedListView(lvHandle);
            ExtendedListView_AddFallbackColumns(lvHandle, 3, fallbackColumns);

            if (NT_SUCCESS(PhInitializeMappedImageImports(&imports, &PvMappedImage)))
            {
                for (i = 0; i < imports.NumberOfDlls; i++)
                {
                    if (NT_SUCCESS(PhGetMappedImageImportDll(&imports, i, &importDll)))
                    {
                        for (j = 0; j < importDll.NumberOfEntries; j++)
                        {
                            if (NT_SUCCESS(PhGetMappedImageImportEntry(&importDll, j, &importEntry)))
                            {
                                INT lvItemIndex;
                                PPH_STRING name;
                                WCHAR number[PH_INT32_STR_LEN_1];

                                name = PhCreateStringFromAnsi(importDll.Name);
                                lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, name->Buffer, NULL);
                                PhDereferenceObject(name);

                                if (importEntry.Name)
                                {
                                    name = PhCreateStringFromAnsi(importEntry.Name);
                                    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, name->Buffer);
                                    PhDereferenceObject(name);

                                    PhPrintUInt32(number, importEntry.NameHint);
                                    PhSetListViewSubItem(lvHandle, lvItemIndex, 2, number);
                                }
                                else
                                {
                                    name = PhFormatString(L"(Ordinal %u)", importEntry.Ordinal);
                                    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, name->Buffer);
                                    PhDereferenceObject(name);
                                }
                            }
                        }
                    }
                }
            }

            ExtendedListView_SortItems(lvHandle);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PvpPeExportsDlgProc(
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
            HWND lvHandle;
            PH_MAPPED_IMAGE_EXPORTS exports;
            PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
            PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;
            ULONG i;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 50, L"Ordinal");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"VA");
            PhSetExtendedListView(lvHandle);

            if (NT_SUCCESS(PhInitializeMappedImageExports(&exports, &PvMappedImage)))
            {
                for (i = 0; i < exports.NumberOfEntries; i++)
                {
                    if (
                        NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)) &&
                        NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction)) 
                        )
                    {
                        INT lvItemIndex;
                        PPH_STRING name;
                        WCHAR number[PH_INT32_STR_LEN_1];
                        WCHAR pointer[PH_PTR_STR_LEN_1];

                        if (exportEntry.Name)
                        {
                            name = PhCreateStringFromAnsi(exportEntry.Name);
                            lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, name->Buffer, NULL);
                            PhDereferenceObject(name);
                        }
                        else
                        {
                            lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, L"(unnamed)", NULL);
                        }

                        PhPrintUInt32(number, exportEntry.Ordinal);
                        PhSetListViewSubItem(lvHandle, lvItemIndex, 1, number);

                        if (!exportFunction.ForwardedName)
                        {
                            PhPrintPointer(pointer, PTR_SUB_OFFSET(exportFunction.Function, PvMappedImage.ViewBase));
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, pointer);
                        }
                        else
                        {
                            name = PhCreateStringFromAnsi(exportFunction.ForwardedName);
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, name->Buffer);
                            PhDereferenceObject(name);
                        }
                    }
                }
            }

            ExtendedListView_SortItems(lvHandle);
        }
        break;
    }

    return FALSE;
}

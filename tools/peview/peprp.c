/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010-2011 wj32
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
#include <cpysave.h>
#include <verify.h>
#include <shlobj.h>

#define PVM_CHECKSUM_DONE (WM_APP + 1)
#define PVM_VERIFY_DONE (WM_APP + 2)

INT_PTR CALLBACK PvpPeGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeImportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeExportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeLoadConfigDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PvpPeClrDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

PH_MAPPED_IMAGE PvMappedImage;
PIMAGE_COR20_HEADER PvImageCor20Header;

HICON PvImageLargeIcon;
PH_IMAGE_VERSION_INFO PvImageVersionInfo;
VERIFY_RESULT PvImageVerifyResult;
PPH_STRING PvImageSignerName;

VOID PvPeProperties(
    VOID
    )
{
    NTSTATUS status;
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[5];
    PH_MAPPED_IMAGE_IMPORTS imports;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PIMAGE_DATA_DIRECTORY entry;

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
    if ((NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)) && imports.NumberOfDlls != 0) ||
        (NT_SUCCESS(PhGetMappedImageDelayImports(&imports, &PvMappedImage)) && imports.NumberOfDlls != 0))
    {
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PEIMPORTS);
        propSheetPage.pfnDlgProc = PvpPeImportsDlgProc;
        pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
    }

    // Exports page
    if (NT_SUCCESS(PhGetMappedImageExports(&exports, &PvMappedImage)) && exports.NumberOfEntries != 0)
    {
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PEEXPORTS);
        propSheetPage.pfnDlgProc = PvpPeExportsDlgProc;
        pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
    }

    // Load Config page
    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &entry)) && entry->VirtualAddress)
    {
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PELOADCONFIG);
        propSheetPage.pfnDlgProc = PvpPeLoadConfigDlgProc;
        pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
    }

    // CLR page
    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &entry)) &&
        entry->VirtualAddress &&
        (PvImageCor20Header = PhMappedImageRvaToVa(&PvMappedImage, entry->VirtualAddress, NULL)))
    {
        status = STATUS_SUCCESS;

        __try
        {
            PhProbeAddress(PvImageCor20Header, sizeof(IMAGE_COR20_HEADER),
                PvMappedImage.ViewBase, PvMappedImage.Size, 4);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }

        if (NT_SUCCESS(status))
        {
            memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
            propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
            propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PECLR);
            propSheetPage.pfnDlgProc = PvpPeClrDlgProc;
            pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
        }
    }

    PropertySheet(&propSheetHeader);

    PhUnloadMappedImage(&PvMappedImage);
}

static NTSTATUS CheckSumImageThreadStart(
    _In_ PVOID Parameter
    )
{
    HWND windowHandle;
    ULONG checkSum;

    windowHandle = Parameter;
    checkSum = PhCheckSumMappedImage(&PvMappedImage);

    PostMessage(
        windowHandle,
        PVM_CHECKSUM_DONE,
        checkSum,
        0
        );

    return STATUS_SUCCESS;
}

VERIFY_RESULT PvpVerifyFileWithAdditionalCatalog(
    _In_ PPH_STRING FileName,
    _In_ ULONG Flags,
    _In_opt_ HWND hWnd,
    _Out_opt_ PPH_STRING *SignerName
    )
{
    static PH_STRINGREF codeIntegrityFileName = PH_STRINGREF_INIT(L"\\AppxMetadata\\CodeIntegrity.cat");

    VERIFY_RESULT result;
    PH_VERIFY_FILE_INFO info;
    PPH_STRING windowsAppsPath;
    PPH_STRING additionalCatalogFileName = NULL;
    PCERT_CONTEXT *signatures;
    ULONG numberOfSignatures;

    memset(&info, 0, sizeof(PH_VERIFY_FILE_INFO));
    info.FileName = FileName->Buffer;
    info.Flags = Flags;
    info.hWnd = hWnd;

    windowsAppsPath = PhGetKnownLocation(CSIDL_PROGRAM_FILES, L"\\WindowsApps\\");

    if (windowsAppsPath)
    {
        if (PhStartsWithStringRef(&FileName->sr, &windowsAppsPath->sr, TRUE))
        {
            PH_STRINGREF remainingFileName;
            ULONG_PTR indexOfBackslash;
            PH_STRINGREF baseFileName;

            remainingFileName = FileName->sr;
            PhSkipStringRef(&remainingFileName, windowsAppsPath->Length);
            indexOfBackslash = PhFindCharInStringRef(&remainingFileName, '\\', FALSE);

            if (indexOfBackslash != -1)
            {
                baseFileName.Buffer = FileName->Buffer;
                baseFileName.Length = windowsAppsPath->Length + indexOfBackslash * sizeof(WCHAR);
                additionalCatalogFileName = PhConcatStringRef2(&baseFileName, &codeIntegrityFileName);
            }
        }

        PhDereferenceObject(windowsAppsPath);
    }

    if (additionalCatalogFileName)
    {
        info.NumberOfCatalogFileNames = 1;
        info.CatalogFileNames = &additionalCatalogFileName->Buffer;
    }

    if (!NT_SUCCESS(PhVerifyFileEx(&info, &result, &signatures, &numberOfSignatures)))
    {
        result = VrNoSignature;
        signatures = NULL;
        numberOfSignatures = 0;
    }

    if (additionalCatalogFileName)
        PhDereferenceObject(additionalCatalogFileName);

    if (SignerName)
    {
        if (numberOfSignatures != 0)
            *SignerName = PhGetSignerNameFromCertificate(signatures[0]);
        else
            *SignerName = NULL;
    }

    PhFreeVerifySignatures(signatures, numberOfSignatures);

    return result;
}

static NTSTATUS VerifyImageThreadStart(
    _In_ PVOID Parameter
    )
{
    HWND windowHandle;

    windowHandle = Parameter;
    PvImageVerifyResult = PvpVerifyFileWithAdditionalCatalog(PvFileName, PH_VERIFY_PREVENT_NETWORK_ACCESS, NULL, &PvImageSignerName);
    PostMessage(windowHandle, PVM_VERIFY_DONE, 0, 0);

    return STATUS_SUCCESS;
}

FORCEINLINE PWSTR PvpGetStringOrNa(
    _In_ PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return L"N/A";
}

INT_PTR CALLBACK PvpPeGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
            PH_STRING_BUILDER stringBuilder;

            PhCenterWindow(GetParent(hwndDlg), NULL);

            // File version information

            {
                PhInitializeImageVersionInfo(&PvImageVersionInfo, PvFileName->Buffer);

                if (ExtractIconEx(
                    PvFileName->Buffer,
                    0,
                    &PvImageLargeIcon,
                    NULL,
                    1
                    ) == 0)
                {
                    PvImageLargeIcon = PhGetFileShellIcon(PvFileName->Buffer, NULL, TRUE);
                }

                SendMessage(GetDlgItem(hwndDlg, IDC_FILEICON), STM_SETICON, (WPARAM)PvImageLargeIcon, 0);

                SetDlgItemText(hwndDlg, IDC_NAME, PvpGetStringOrNa(PvImageVersionInfo.FileDescription));
                string = PhConcatStrings2(L"(Verifying...) ", PvpGetStringOrNa(PvImageVersionInfo.CompanyName));
                SetDlgItemText(hwndDlg, IDC_COMPANYNAME, string->Buffer);
                PhDereferenceObject(string);
                SetDlgItemText(hwndDlg, IDC_VERSION, PvpGetStringOrNa(PvImageVersionInfo.FileVersion));

                PhQueueItemGlobalWorkQueue(VerifyImageThreadStart, hwndDlg);
            }

            // PE properties

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
            case IMAGE_FILE_MACHINE_ARMNT:
                type = L"ARM Thumb-2";
                break;
            default:
                type = L"Unknown";
                break;
            }

            SetDlgItemText(hwndDlg, IDC_TARGETMACHINE, type);

            {
                LARGE_INTEGER time;
                SYSTEMTIME systemTime;

                RtlSecondsSince1970ToTime(PvMappedImage.NtHeaders->FileHeader.TimeDateStamp, &time);
                PhLargeIntegerToLocalSystemTime(&systemTime, &time);

                string = PhFormatDateTime(&systemTime);
                SetDlgItemText(hwndDlg, IDC_TIMESTAMP, string->Buffer);
                PhDereferenceObject(string);
            }

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                string = PhFormatString(L"0x%Ix", PvMappedImage.NtHeaders->OptionalHeader.ImageBase);
            }
            else
            {
                string = PhFormatString(L"0x%I64x", ((PIMAGE_OPTIONAL_HEADER64)&PvMappedImage.NtHeaders->OptionalHeader)->ImageBase);
            }

            SetDlgItemText(hwndDlg, IDC_IMAGEBASE, string->Buffer);
            PhDereferenceObject(string);

            string = PhFormatString(L"0x%Ix (verifying...)", PvMappedImage.NtHeaders->OptionalHeader.CheckSum); // same for 32-bit and 64-bit images
            SetDlgItemText(hwndDlg, IDC_CHECKSUM, string->Buffer);
            PhDereferenceObject(string);

            PhQueueItemGlobalWorkQueue(CheckSumImageThreadStart, hwndDlg);

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
            case IMAGE_SUBSYSTEM_OS2_CUI:
                type = L"OS/2 CUI";
                break;
            case IMAGE_SUBSYSTEM_POSIX_CUI:
                type = L"POSIX CUI";
                break;
            case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:
                type = L"Windows CE CUI";
                break;
            case IMAGE_SUBSYSTEM_EFI_APPLICATION:
                type = L"EFI Application";
                break;
            case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
                type = L"EFI Boot Service Driver";
                break;
            case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
                type = L"EFI Runtime Driver";
                break;
            case IMAGE_SUBSYSTEM_EFI_ROM:
                type = L"EFI ROM";
                break;
            case IMAGE_SUBSYSTEM_XBOX:
                type = L"Xbox";
                break;
            case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION:
                type = L"Windows Boot Application";
                break;
            default:
                type = L"Unknown";
                break;
            }

            SetDlgItemText(hwndDlg, IDC_SUBSYSTEM, type);

            string = PhFormatString(
                L"%u.%u",
                PvMappedImage.NtHeaders->OptionalHeader.MajorSubsystemVersion, // same for 32-bit and 64-bit images
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

            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA)
                PhAppendStringBuilder2(&stringBuilder, L"High entropy VA, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)
                PhAppendStringBuilder2(&stringBuilder, L"Dynamic base, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY)
                PhAppendStringBuilder2(&stringBuilder, L"Force integrity check, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NX_COMPAT)
                PhAppendStringBuilder2(&stringBuilder, L"NX compatible, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_ISOLATION)
                PhAppendStringBuilder2(&stringBuilder, L"No isolation, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_SEH)
                PhAppendStringBuilder2(&stringBuilder, L"No SEH, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_BIND)
                PhAppendStringBuilder2(&stringBuilder, L"Do not bind, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_APPCONTAINER)
                PhAppendStringBuilder2(&stringBuilder, L"AppContainer, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_WDM_DRIVER)
                PhAppendStringBuilder2(&stringBuilder, L"WDM driver, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_GUARD_CF)
                PhAppendStringBuilder2(&stringBuilder, L"Control Flow Guard, ");
            if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE)
                PhAppendStringBuilder2(&stringBuilder, L"Terminal server aware, ");

            if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                PhRemoveEndStringBuilder(&stringBuilder, 2);

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

                if (PhCopyStringZFromBytes(PvMappedImage.Sections[i].Name,
                    IMAGE_SIZEOF_SHORT_NAME, sectionName, 9, NULL))
                {
                    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, sectionName, NULL);

                    PhPrintPointer(pointer, UlongToPtr(PvMappedImage.Sections[i].VirtualAddress));
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, pointer);

                    PhPrintPointer(pointer, UlongToPtr(PvMappedImage.Sections[i].SizeOfRawData));
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 2, pointer);
                }
            }
        }
        break;
    case PVM_CHECKSUM_DONE:
        {
            PPH_STRING string;
            ULONG headerCheckSum;
            ULONG realCheckSum;

            headerCheckSum = PvMappedImage.NtHeaders->OptionalHeader.CheckSum; // same for 32-bit and 64-bit images
            realCheckSum = (ULONG)wParam;

            if (headerCheckSum == 0)
            {
                // Some executables, like .NET ones, don't have a check sum.
                string = PhFormatString(L"0x0 (real 0x%Ix)", realCheckSum);
                SetDlgItemText(hwndDlg, IDC_CHECKSUM, string->Buffer);
                PhDereferenceObject(string);
            }
            else if (headerCheckSum == realCheckSum)
            {
                string = PhFormatString(L"0x%Ix (correct)", headerCheckSum);
                SetDlgItemText(hwndDlg, IDC_CHECKSUM, string->Buffer);
                PhDereferenceObject(string);
            }
            else
            {
                string = PhFormatString(L"0x%Ix (incorrect, real 0x%Ix)", headerCheckSum, realCheckSum);
                SetDlgItemText(hwndDlg, IDC_CHECKSUM, string->Buffer);
                PhDereferenceObject(string);
            }
        }
        break;
    case PVM_VERIFY_DONE:
        {
            PPH_STRING string;

            if (PvImageVerifyResult == VrTrusted)
            {
                if (PvImageSignerName)
                {
                    string = PhFormatString(L"<a>(Verified) %s</a>", PvImageSignerName->Buffer);
                    SetDlgItemText(hwndDlg, IDC_COMPANYNAME_LINK, string->Buffer);
                    PhDereferenceObject(string);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_COMPANYNAME), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_COMPANYNAME_LINK), SW_SHOW);
                }
                else
                {
                    string = PhConcatStrings2(L"(Verified) ", PhGetStringOrEmpty(PvImageVersionInfo.CompanyName));
                    SetDlgItemText(hwndDlg, IDC_COMPANYNAME, string->Buffer);
                    PhDereferenceObject(string);
                }
            }
            else if (PvImageVerifyResult != VrUnknown)
            {
                string = PhConcatStrings2(L"(UNVERIFIED) ", PhGetStringOrEmpty(PvImageVersionInfo.CompanyName));
                SetDlgItemText(hwndDlg, IDC_COMPANYNAME, string->Buffer);
                PhDereferenceObject(string);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_COMPANYNAME, PvpGetStringOrNa(PvImageVersionInfo.CompanyName));
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_CLICK:
                {
                    switch (header->idFrom)
                    {
                    case IDC_COMPANYNAME_LINK:
                        {
                            PvpVerifyFileWithAdditionalCatalog(PvFileName, PH_VERIFY_VIEW_PROPERTIES, hwndDlg, NULL);
                        }
                        break;
                    }
                }
                break;
            }

            PvHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    }

    return FALSE;
}

VOID PvpProcessImports(
    _In_ HWND ListViewHandle,
    _In_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ BOOLEAN DelayImports
    )
{
    PH_MAPPED_IMAGE_IMPORT_DLL importDll;
    PH_MAPPED_IMAGE_IMPORT_ENTRY importEntry;
    ULONG i;
    ULONG j;

    for (i = 0; i < Imports->NumberOfDlls; i++)
    {
        if (NT_SUCCESS(PhGetMappedImageImportDll(Imports, i, &importDll)))
        {
            for (j = 0; j < importDll.NumberOfEntries; j++)
            {
                if (NT_SUCCESS(PhGetMappedImageImportEntry(&importDll, j, &importEntry)))
                {
                    INT lvItemIndex;
                    PPH_STRING name;
                    WCHAR number[PH_INT32_STR_LEN_1];

                    if (!DelayImports)
                        name = PhZeroExtendToUtf16(importDll.Name);
                    else
                        name = PhFormatString(L"%S (Delay)", importDll.Name);

                    lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, name->Buffer, NULL);
                    PhDereferenceObject(name);

                    if (importEntry.Name)
                    {
                        name = PhZeroExtendToUtf16(importEntry.Name);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, name->Buffer);
                        PhDereferenceObject(name);

                        PhPrintUInt32(number, importEntry.NameHint);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, number);
                    }
                    else
                    {
                        name = PhFormatString(L"(Ordinal %u)", importEntry.Ordinal);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, name->Buffer);
                        PhDereferenceObject(name);
                    }
                }
            }
        }
    }
}

INT_PTR CALLBACK PvpPeImportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG fallbackColumns[] = { 0, 1, 2 };
            HWND lvHandle;
            PH_MAPPED_IMAGE_IMPORTS imports;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 130, L"DLL");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 210, L"Name");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"Hint");
            PhSetExtendedListView(lvHandle);
            ExtendedListView_AddFallbackColumns(lvHandle, 3, fallbackColumns);

            if (NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)))
            {
                PvpProcessImports(lvHandle, &imports, FALSE);
            }

            if (NT_SUCCESS(PhGetMappedImageDelayImports(&imports, &PvMappedImage)))
            {
                PvpProcessImports(lvHandle, &imports, TRUE);
            }

            ExtendedListView_SortItems(lvHandle);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_CTLCOLORDLG:
        {
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PvpPeExportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 220, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 50, L"Ordinal");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 120, L"VA");
            PhSetExtendedListView(lvHandle);

            if (NT_SUCCESS(PhGetMappedImageExports(&exports, &PvMappedImage)))
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
                            name = PhZeroExtendToUtf16(exportEntry.Name);
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
                            if ((ULONG_PTR)exportFunction.Function >= (ULONG_PTR)PvMappedImage.ViewBase)
                                PhPrintPointer(pointer, PTR_SUB_OFFSET(exportFunction.Function, PvMappedImage.ViewBase));
                            else
                                PhPrintPointer(pointer, exportFunction.Function);

                            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, pointer);
                        }
                        else
                        {
                            name = PhZeroExtendToUtf16(exportFunction.ForwardedName);
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, name->Buffer);
                            PhDereferenceObject(name);
                        }
                    }
                }
            }

            ExtendedListView_SortItems(lvHandle);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_CTLCOLORDLG:
        {
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PvpPeLoadConfigDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PH_AUTO_POOL autoPool;
            HWND lvHandle;
            PIMAGE_LOAD_CONFIG_DIRECTORY32 config32;
            PIMAGE_LOAD_CONFIG_DIRECTORY64 config64;
            PPH_STRING string;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 220, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 170, L"Value");

#define ADD_VALUE(Name, Value) \
    do { \
        INT lvItemIndex; \
        \
        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, Name, NULL); \
        PhSetListViewSubItem(lvHandle, lvItemIndex, 1, Value); \
    } while (0)

#define ADD_VALUES(Config) \
    do { \
        { \
            LARGE_INTEGER time; \
            SYSTEMTIME systemTime; \
            \
            RtlSecondsSince1970ToTime((Config)->TimeDateStamp, &time); \
            PhLargeIntegerToLocalSystemTime(&systemTime, &time); \
            \
            string = PhFormatDateTime(&systemTime); \
            ADD_VALUE(L"Time stamp", string->Buffer); \
            PhDereferenceObject(string); \
        } \
        \
        ADD_VALUE(L"Version", PhaFormatString(L"%u.%u", (Config)->MajorVersion, (Config)->MinorVersion)->Buffer); \
        ADD_VALUE(L"Global flags to clear", PhaFormatString(L"0x%x", (Config)->GlobalFlagsClear)->Buffer); \
        ADD_VALUE(L"Global flags to set", PhaFormatString(L"0x%x", (Config)->GlobalFlagsSet)->Buffer); \
        ADD_VALUE(L"Critical section default timeout", PhaFormatUInt64((Config)->CriticalSectionDefaultTimeout, TRUE)->Buffer); \
        ADD_VALUE(L"De-commit free block threshold", PhaFormatUInt64((Config)->DeCommitFreeBlockThreshold, TRUE)->Buffer); \
        ADD_VALUE(L"De-commit total free threshold", PhaFormatUInt64((Config)->DeCommitTotalFreeThreshold, TRUE)->Buffer); \
        ADD_VALUE(L"LOCK prefix table", PhaFormatString(L"0x%Ix", (Config)->LockPrefixTable)->Buffer); \
        ADD_VALUE(L"Maximum allocation size", PhaFormatString(L"0x%Ix", (Config)->MaximumAllocationSize)->Buffer); \
        ADD_VALUE(L"Virtual memory threshold", PhaFormatString(L"0x%Ix", (Config)->VirtualMemoryThreshold)->Buffer); \
        ADD_VALUE(L"Process affinity mask", PhaFormatString(L"0x%Ix", (Config)->ProcessAffinityMask)->Buffer); \
        ADD_VALUE(L"Process heap flags", PhaFormatString(L"0x%Ix", (Config)->ProcessHeapFlags)->Buffer); \
        ADD_VALUE(L"CSD version", PhaFormatString(L"%u", (Config)->CSDVersion)->Buffer); \
        ADD_VALUE(L"Edit list", PhaFormatString(L"0x%Ix", (Config)->EditList)->Buffer); \
        ADD_VALUE(L"Security cookie", PhaFormatString(L"0x%Ix", (Config)->SecurityCookie)->Buffer); \
        ADD_VALUE(L"SEH handler table", PhaFormatString(L"0x%Ix", (Config)->SEHandlerTable)->Buffer); \
        ADD_VALUE(L"SEH handler count", PhaFormatUInt64((Config)->SEHandlerCount, TRUE)->Buffer); \
    } while (0)

            PhInitializeAutoPool(&autoPool);

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                if (NT_SUCCESS(PhGetMappedImageLoadConfig32(&PvMappedImage, &config32)))
                {
                    ADD_VALUES(config32);
                }
            }
            else
            {
                if (NT_SUCCESS(PhGetMappedImageLoadConfig64(&PvMappedImage, &config64)))
                {
                    ADD_VALUES(config64);
                }
            }

            PhDeleteAutoPool(&autoPool);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_CTLCOLORDLG:
        {
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PvpPeClrDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PH_STRING_BUILDER stringBuilder;
            PPH_STRING string;
            PVOID metaData;
            ULONG versionStringLength;

            string = PhFormatString(L"%u.%u", PvImageCor20Header->MajorRuntimeVersion,
                PvImageCor20Header->MinorRuntimeVersion);
            SetDlgItemText(hwndDlg, IDC_RUNTIMEVERSION, string->Buffer);
            PhDereferenceObject(string);

            PhInitializeStringBuilder(&stringBuilder, 256);

            if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_ILONLY)
                PhAppendStringBuilder2(&stringBuilder, L"IL only, ");
            if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_32BITREQUIRED)
                PhAppendStringBuilder2(&stringBuilder, L"32-bit only, ");
            if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_32BITPREFERRED)
                PhAppendStringBuilder2(&stringBuilder, L"32-bit preferred, ");
            if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_IL_LIBRARY)
                PhAppendStringBuilder2(&stringBuilder, L"IL library, ");

            if (PvImageCor20Header->StrongNameSignature.VirtualAddress != 0 && PvImageCor20Header->StrongNameSignature.Size != 0)
            {
                if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_STRONGNAMESIGNED)
                    PhAppendStringBuilder2(&stringBuilder, L"Strong-name signed, ");
                else
                    PhAppendStringBuilder2(&stringBuilder, L"Strong-name delay signed, ");
            }

            if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_NATIVE_ENTRYPOINT)
                PhAppendStringBuilder2(&stringBuilder, L"Native entry-point, ");
            if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_TRACKDEBUGDATA)
                PhAppendStringBuilder2(&stringBuilder, L"Track debug data, ");

            if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                PhRemoveEndStringBuilder(&stringBuilder, 2);

            SetDlgItemText(hwndDlg, IDC_FLAGS, stringBuilder.String->Buffer);
            PhDeleteStringBuilder(&stringBuilder);

            metaData = PhMappedImageRvaToVa(&PvMappedImage, PvImageCor20Header->MetaData.VirtualAddress, NULL);

            if (metaData)
            {
                __try
                {
                    PhProbeAddress(metaData, PvImageCor20Header->MetaData.Size, PvMappedImage.ViewBase, PvMappedImage.Size, 4);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    metaData = NULL;
                }
            }

            versionStringLength = 0;

            if (metaData)
            {
                // Skip 12 bytes.
                // First 4 bytes contains the length of the version string.
                // The version string follows.
                versionStringLength = *(PULONG)((PCHAR)metaData + 12);

                // Make sure the length is valid.
                if (versionStringLength >= 0x100)
                    versionStringLength = 0;
            }

            if (versionStringLength != 0)
            {
                string = PhZeroExtendToUtf16Ex((PCHAR)metaData + 12 + 4, versionStringLength);
                SetDlgItemText(hwndDlg, IDC_VERSIONSTRING, string->Buffer);
                PhDereferenceObject(string);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_VERSIONSTRING, L"N/A");
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_RUNTIMEVERSION));
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010-2011 wj32
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
#include <workqueue.h>
#include <cpysave.h>
#include <verify.h>
#include <shlobj.h>
#include <shellapi.h>

#define PVM_CHECKSUM_DONE (WM_APP + 1)
#define PVM_VERIFY_DONE (WM_APP + 2)

BOOLEAN PvpLoadDbgHelp(
    _Inout_ PPH_SYMBOL_PROVIDER *SymbolProvider
    );

INT_PTR CALLBACK PvpPeGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

PH_MAPPED_IMAGE PvMappedImage;
PIMAGE_COR20_HEADER PvImageCor20Header = NULL;
PPH_SYMBOL_PROVIDER PvSymbolProvider = NULL;
static HICON PvImageLargeIcon;
static PH_IMAGE_VERSION_INFO PvImageVersionInfo;
static VERIFY_RESULT PvImageVerifyResult;
static PPH_STRING PvImageSignerName;

VOID PvPeProperties(
    VOID
    )
{
    NTSTATUS status;
    PPV_PROPCONTEXT propContext;
    PH_MAPPED_IMAGE_IMPORTS imports;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PIMAGE_DATA_DIRECTORY entry;

    if (!NT_SUCCESS(status = PhLoadMappedImage(
        PvFileName->Buffer, 
        NULL, 
        TRUE, 
        &PvMappedImage
        )))
    {
        PhShowStatus(NULL, L"Unable to load the PE file", status, 0);
        return;
    }

    if (PvpLoadDbgHelp(&PvSymbolProvider))
    {
        // Load current PE pdb
        // TODO: Move into seperate thread.
        PhLoadModuleSymbolProvider(
            PvSymbolProvider,
            PvFileName->Buffer,
            (ULONG64)PvMappedImage.NtHeaders->OptionalHeader.ImageBase,
            PvMappedImage.NtHeaders->OptionalHeader.SizeOfImage
            );
    }

    if (propContext = PvCreatePropContext(PvFileName))
    {
        PPV_PROPPAGECONTEXT newPage;

        // General page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_PEGENERAL), 
            PvpPeGeneralDlgProc, 
            NULL
            );
        PvAddPropPage(propContext, newPage);

        // Load Config page
        if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &entry)) && entry->VirtualAddress)
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PELOADCONFIG),
                PvpPeLoadConfigDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // Imports page
        if ((NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)) && imports.NumberOfDlls != 0) ||
            (NT_SUCCESS(PhGetMappedImageDelayImports(&imports, &PvMappedImage)) && imports.NumberOfDlls != 0))
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PEIMPORTS),
                PvpPeImportsDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // Exports page
        if (NT_SUCCESS(PhGetMappedImageExports(&exports, &PvMappedImage)) && exports.NumberOfEntries != 0)
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PEEXPORTS),
                PvpPeExportsDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // CLR page
        if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &entry)) &&
            entry->VirtualAddress &&
            (PvImageCor20Header = PhMappedImageRvaToVa(&PvMappedImage, entry->VirtualAddress, NULL)))
        {
            status = STATUS_SUCCESS;

            __try
            {
                PhProbeAddress(
                    PvImageCor20Header, 
                    sizeof(IMAGE_COR20_HEADER),
                    PvMappedImage.ViewBase, 
                    PvMappedImage.Size, 
                    4
                    );
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }

            if (NT_SUCCESS(status))
            {
                newPage = PvCreatePropPageContext(
                    MAKEINTRESOURCE(IDD_PECLR),
                    PvpPeClrDlgProc,
                    NULL
                    );
                PvAddPropPage(propContext, newPage);
            }
        }

        // CFG page
        if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_GUARD_CF)
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PECFG),
                PvpPeCgfDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // Symbols page
        if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_DEBUG, &entry)) && entry->VirtualAddress)
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PESYMBOLS),
                PvpSymbolsDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        PhModalPropertySheet(&propContext->PropSheetHeader);

        PhDereferenceObject(propContext);
    }

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
    if (!PhIsNullOrEmptyString(String))
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
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;
            ULONG i;
            PPH_STRING string;
            PWSTR type;
            PH_STRING_BUILDER stringBuilder;

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

                PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), VerifyImageThreadStart, hwndDlg);
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
                string = PhFormatString(L"0x%I32x", ((PIMAGE_OPTIONAL_HEADER32)&PvMappedImage.NtHeaders->OptionalHeader)->ImageBase);
            }
            else
            {
                string = PhFormatString(L"0x%I64x", ((PIMAGE_OPTIONAL_HEADER64)&PvMappedImage.NtHeaders->OptionalHeader)->ImageBase);
            }

            SetDlgItemText(hwndDlg, IDC_IMAGEBASE, string->Buffer);
            PhDereferenceObject(string);

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                string = PhFormatString(L"0x%I32x", ((PIMAGE_OPTIONAL_HEADER32)&PvMappedImage.NtHeaders->OptionalHeader)->AddressOfEntryPoint);
            }
            else
            {
                string = PhFormatString(L"0x%I64x", ((PIMAGE_OPTIONAL_HEADER64)&PvMappedImage.NtHeaders->OptionalHeader)->AddressOfEntryPoint);
            }

            SetDlgItemText(hwndDlg, IDC_ENTRYPOINT, string->Buffer);
            PhDereferenceObject(string);

            string = PhFormatString(L"0x%Ix (verifying...)", PvMappedImage.NtHeaders->OptionalHeader.CheckSum); // same for 32-bit and 64-bit images
            SetDlgItemText(hwndDlg, IDC_CHECKSUM, string->Buffer);
            PhDereferenceObject(string);

            PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), CheckSumImageThreadStart, hwndDlg);

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
            SetDlgItemText(hwndDlg, IDC_SUBSYSTEMVERSION, PhaFormatString(
                L"%u.%u",
                PvMappedImage.NtHeaders->OptionalHeader.MajorSubsystemVersion, // same for 32-bit and 64-bit images
                PvMappedImage.NtHeaders->OptionalHeader.MinorSubsystemVersion
                )->Buffer);

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
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
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
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhaFormatSize(PvMappedImage.Sections[i].SizeOfRawData, -1)->Buffer);
                }
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILE),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_NAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CHARACTERISTICS),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST),
                    dialogItem, PH_ANCHOR_ALL);

                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
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

VOID PvpLoadDbgHelpFromPath(
    _In_ PWSTR DbgHelpPath
    )
{
    HMODULE dbghelpModule;

    if (dbghelpModule = LoadLibrary(DbgHelpPath))
    {
        PPH_STRING fullDbghelpPath;
        ULONG indexOfFileName;
        PH_STRINGREF dbghelpFolder;
        PPH_STRING symsrvPath;

        fullDbghelpPath = PhGetDllFileName(dbghelpModule, &indexOfFileName);

        if (fullDbghelpPath)
        {
            if (indexOfFileName != 0)
            {
                static PH_STRINGREF symsrvString = PH_STRINGREF_INIT(L"\\symsrv.dll");

                dbghelpFolder.Buffer = fullDbghelpPath->Buffer;
                dbghelpFolder.Length = indexOfFileName * sizeof(WCHAR);

                symsrvPath = PhConcatStringRef2(&dbghelpFolder, &symsrvString);

                LoadLibrary(symsrvPath->Buffer);

                PhDereferenceObject(symsrvPath);
            }

            PhDereferenceObject(fullDbghelpPath);
        }
    }
    else
    {
        dbghelpModule = LoadLibrary(L"dbghelp.dll");
    }

    PhSymbolProviderCompleteInitialization(dbghelpModule);
}

BOOLEAN PvpLoadDbgHelp(
    _Inout_ PPH_SYMBOL_PROVIDER *SymbolProvider
    )
{
    static UNICODE_STRING symbolPathVarName = RTL_CONSTANT_STRING(L"_NT_SYMBOL_PATH");
    PPH_STRING symbolSearchPath;
    PPH_SYMBOL_PROVIDER symbolProvider;
    WCHAR buffer[512] = L"";
    UNICODE_STRING symbolPathUs =
    {
        .Buffer = buffer,
        .Length = sizeof(buffer) - sizeof(UNICODE_NULL),
        .MaximumLength = sizeof(buffer)
    };

    if (!PhSymbolProviderInitialization())
        return FALSE;

    PvpLoadDbgHelpFromPath(L"C:\\Program Files (x86)\\Windows Kits\\10\\Debuggers\\x64\\dbghelp.dll");
    symbolProvider = PhCreateSymbolProvider(NULL);

    // Load symbol path from _NT_SYMBOL_PATH if configured by the user.    
    if (NT_SUCCESS(RtlQueryEnvironmentVariable_U(NULL, &symbolPathVarName, &symbolPathUs)))
    {
        symbolSearchPath = PhFormatString(L"SRV*%s*http://msdl.microsoft.com/download/symbols", symbolPathUs.Buffer);
    }
    else
    {
        symbolSearchPath = PhCreateString(L"SRV**http://msdl.microsoft.com/download/symbols");
    }

    PhSetSearchPathSymbolProvider(symbolProvider, symbolSearchPath->Buffer);
    PhDereferenceObject(symbolSearchPath);

    *SymbolProvider = symbolProvider;
    return TRUE;
}
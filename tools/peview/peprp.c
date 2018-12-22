/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2017-2018 dmex
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
HICON PvImageSmallIcon = NULL;
HICON PvImageLargeIcon = NULL;
static PH_IMAGE_VERSION_INFO PvImageVersionInfo;
static VERIFY_RESULT PvImageVerifyResult;
static PPH_STRING PvImageSignerName;

VOID PvPeProperties(
    VOID
    )
{
    PPV_PROPCONTEXT propContext;
    PH_MAPPED_IMAGE_IMPORTS imports;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PIMAGE_DATA_DIRECTORY entry;

    if (!PhExtractIcon(PvFileName->Buffer, &PvImageLargeIcon, &PvImageSmallIcon))
    {
        PhGetStockApplicationIcon(&PvImageSmallIcon, &PvImageLargeIcon);
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

        // Resources page
        if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_RESOURCE, &entry)) && entry->VirtualAddress)
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PERESOURCES),
                PvpPeResourcesDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // CLR page
        if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &entry)) &&
            entry->VirtualAddress &&
            (PvImageCor20Header = PhMappedImageRvaToVa(&PvMappedImage, entry->VirtualAddress, NULL)))
        {
            NTSTATUS status = STATUS_SUCCESS;

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

        // Properties page
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PEPROPSTORAGE),
                PvpPePropStoreDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // Extended attributes page
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PEATTR),
                PvpPeExtendedAttributesDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // Streams page
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PESTREAMS),
                PvpPeStreamsDlgProc,
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
    static PH_STRINGREF windowsAppsPathSr = PH_STRINGREF_INIT(L"%ProgramFiles%\\WindowsApps\\");
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

    if (windowsAppsPath = PhExpandEnvironmentStrings(&windowsAppsPathSr))
    {
        if (PhStartsWithStringRef(&FileName->sr, &windowsAppsPath->sr, TRUE))
        {
            PH_STRINGREF remainingFileName;
            ULONG_PTR indexOfBackslash;
            PH_STRINGREF baseFileName;

            remainingFileName = FileName->sr;
            PhSkipStringRef(&remainingFileName, windowsAppsPath->Length);
            indexOfBackslash = PhFindCharInStringRef(&remainingFileName, OBJ_NAME_PATH_SEPARATOR, FALSE);

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

FORCEINLINE PPH_STRING PvpGetSectionCharacteristics(
    _In_ ULONG Characteristics
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (Characteristics & IMAGE_SCN_TYPE_NO_PAD)
        PhAppendStringBuilder2(&stringBuilder, L"No Padding, ");
    if (Characteristics & IMAGE_SCN_CNT_CODE)
        PhAppendStringBuilder2(&stringBuilder, L"Code, ");
    if (Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
        PhAppendStringBuilder2(&stringBuilder, L"Initialized data, ");
    if (Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
        PhAppendStringBuilder2(&stringBuilder, L"Uninitialized data, ");
    if (Characteristics & IMAGE_SCN_LNK_INFO)
        PhAppendStringBuilder2(&stringBuilder, L"Comments, ");
    if (Characteristics & IMAGE_SCN_LNK_REMOVE)
        PhAppendStringBuilder2(&stringBuilder, L"Excluded, ");
    if (Characteristics & IMAGE_SCN_LNK_COMDAT)
        PhAppendStringBuilder2(&stringBuilder, L"COMDAT, ");
    if (Characteristics & IMAGE_SCN_NO_DEFER_SPEC_EXC)
        PhAppendStringBuilder2(&stringBuilder, L"Speculative exceptions, ");
    if (Characteristics & IMAGE_SCN_GPREL)
        PhAppendStringBuilder2(&stringBuilder, L"GP relative, ");
    if (Characteristics & IMAGE_SCN_MEM_PURGEABLE)
        PhAppendStringBuilder2(&stringBuilder, L"Purgeable, ");
    if (Characteristics & IMAGE_SCN_MEM_LOCKED)
        PhAppendStringBuilder2(&stringBuilder, L"Locked, ");
    if (Characteristics & IMAGE_SCN_MEM_PRELOAD)
        PhAppendStringBuilder2(&stringBuilder, L"Preload, ");
    if (Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL)
        PhAppendStringBuilder2(&stringBuilder, L"Extended relocations, ");
    if (Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
        PhAppendStringBuilder2(&stringBuilder, L"Discardable, ");
    if (Characteristics & IMAGE_SCN_MEM_NOT_CACHED)
        PhAppendStringBuilder2(&stringBuilder, L"Not cachable, ");
    if (Characteristics & IMAGE_SCN_MEM_NOT_PAGED)
        PhAppendStringBuilder2(&stringBuilder, L"Not pageable, ");
    if (Characteristics & IMAGE_SCN_MEM_SHARED)
        PhAppendStringBuilder2(&stringBuilder, L"Shareable, ");
    if (Characteristics & IMAGE_SCN_MEM_EXECUTE)
        PhAppendStringBuilder2(&stringBuilder, L"Executable, ");
    if (Characteristics & IMAGE_SCN_MEM_READ)
        PhAppendStringBuilder2(&stringBuilder, L"Readable, ");
    if (Characteristics & IMAGE_SCN_MEM_WRITE)
        PhAppendStringBuilder2(&stringBuilder, L"Writeable, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, (PVOID)(ULONG_PTR)Characteristics);
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PvpSetPeImageVersionInfo(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING string;

    PhInitializeImageVersionInfo(&PvImageVersionInfo, PvFileName->Buffer);

    Static_SetIcon(GetDlgItem(WindowHandle, IDC_FILEICON), PvImageLargeIcon);

    string = PhConcatStrings2(L"(Verifying...) ", PvpGetStringOrNa(PvImageVersionInfo.CompanyName));

    PhSetDialogItemText(WindowHandle, IDC_NAME, PvpGetStringOrNa(PvImageVersionInfo.FileDescription));
    PhSetDialogItemText(WindowHandle, IDC_COMPANYNAME, string->Buffer);
    PhSetDialogItemText(WindowHandle, IDC_VERSION, PvpGetStringOrNa(PvImageVersionInfo.FileVersion));

    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), VerifyImageThreadStart, WindowHandle);

    PhDereferenceObject(string);
}

#ifndef IMAGE_FILE_MACHINE_CHPE_X86
#define IMAGE_FILE_MACHINE_CHPE_X86 0x3A64 // defined in ntimage.h
#endif

VOID PvpSetPeImageMachineType(
    _In_ HWND WindowHandle
    )
{
    PWSTR type;

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
    case IMAGE_FILE_MACHINE_ARM64:
        type = L"ARM64";
        break;
    case IMAGE_FILE_MACHINE_CHPE_X86:
        type = L"Hybrid PE";
        break;
    default:
        type = L"Unknown";
        break;
    }

    PhSetDialogItemText(WindowHandle, IDC_TARGETMACHINE, type);
}

VOID PvpSetPeImageTimeStamp(
    _In_ HWND WindowHandle
    )
{
    LARGE_INTEGER time;
    SYSTEMTIME systemTime;
    PPH_STRING string;

    RtlSecondsSince1970ToTime(PvMappedImage.NtHeaders->FileHeader.TimeDateStamp, &time);
    PhLargeIntegerToLocalSystemTime(&systemTime, &time);

    string = PhFormatDateTime(&systemTime);
    PhSetDialogItemText(WindowHandle, IDC_TIMESTAMP, string->Buffer);
    PhDereferenceObject(string);
}

VOID PvpSetPeImageBaseAddress(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING string;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        string = PhFormatString(L"0x%I32x", ((PIMAGE_OPTIONAL_HEADER32)&PvMappedImage.NtHeaders->OptionalHeader)->ImageBase);
    else
        string = PhFormatString(L"0x%I64x", ((PIMAGE_OPTIONAL_HEADER64)&PvMappedImage.NtHeaders->OptionalHeader)->ImageBase);

    PhSetDialogItemText(WindowHandle, IDC_IMAGEBASE, string->Buffer);
    PhDereferenceObject(string);
}

VOID PvpSetPeImageEntryPoint(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING string;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        string = PhFormatString(L"0x%I32x", ((PIMAGE_OPTIONAL_HEADER32)&PvMappedImage.NtHeaders->OptionalHeader)->AddressOfEntryPoint);
    else
        string = PhFormatString(L"0x%I64x", ((PIMAGE_OPTIONAL_HEADER64)&PvMappedImage.NtHeaders->OptionalHeader)->AddressOfEntryPoint);

    PhSetDialogItemText(WindowHandle, IDC_ENTRYPOINT, string->Buffer);
    PhDereferenceObject(string);
}

VOID PvpSetPeImageCheckSum(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING string;

    string = PhFormatString(L"0x%Ix (verifying...)", PvMappedImage.NtHeaders->OptionalHeader.CheckSum); // same for 32-bit and 64-bit images

    PhSetDialogItemText(WindowHandle, IDC_CHECKSUM, string->Buffer);

    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), CheckSumImageThreadStart, WindowHandle);

    PhDereferenceObject(string);
}

VOID PvpSetPeImageSubsystem(
    _In_ HWND WindowHandle
    )
{
    PWSTR type;

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

    PhSetDialogItemText(WindowHandle, IDC_SUBSYSTEM, type);
    PhSetDialogItemText(WindowHandle, IDC_SUBSYSTEMVERSION, PhaFormatString(
        L"%u.%u",
        PvMappedImage.NtHeaders->OptionalHeader.MajorSubsystemVersion, // same for 32-bit and 64-bit images
        PvMappedImage.NtHeaders->OptionalHeader.MinorSubsystemVersion
        )->Buffer);
}

VOID PvpSetPeImageCharacteristics(
    _In_ HWND WindowHandle
    )
{
    PH_STRING_BUILDER stringBuilder;

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

    PhSetDialogItemText(WindowHandle, IDC_CHARACTERISTICS, stringBuilder.String->Buffer);
    PhDeleteStringBuilder(&stringBuilder);
}

VOID PvpSetPeImageSections(
    _In_ HWND ListViewHandle
    )
{
    for (ULONG i = 0; i < PvMappedImage.NumberOfSections; i++)
    {
        INT lvItemIndex;
        WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];
        WCHAR pointer[PH_PTR_STR_LEN_1];

        if (PhGetMappedImageSectionName(&PvMappedImage.Sections[i], sectionName, ARRAYSIZE(sectionName), NULL))
        {
            PhPrintPointer(pointer, UlongToPtr(PvMappedImage.Sections[i].VirtualAddress));

            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, sectionName, NULL);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, pointer);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhaFormatSize(PvMappedImage.Sections[i].SizeOfRawData, -1)->Buffer);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PH_AUTO_T(PH_STRING, PvpGetSectionCharacteristics(PvMappedImage.Sections[i].Characteristics))->Buffer);

            //if (PvMappedImage.Sections[i].PointerToRawData && PvMappedImage.Sections[i].SizeOfRawData)
            //{
            //    PH_HASH_CONTEXT hashContext;
            //    PPH_STRING hashString;
            //    UCHAR hash[32];
            //
            //    PhInitializeHash(&hashContext, PhGetIntegerSetting(L"HashAlgorithm"));
            //    PhUpdateHash(&hashContext, PTR_ADD_OFFSET(PvMappedImage.ViewBase, PvMappedImage.Sections[i].PointerToRawData), PvMappedImage.Sections[i].SizeOfRawData);
            //    PhFinalHash(&hashContext, hash, 16, NULL);
            //
            //    hashString = PhBufferToHexString(hash, 16);
            //    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, hashString->Buffer);
            //    PhDereferenceObject(hashString);
            //}
        }
    }
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

            // File version information
            PvpSetPeImageVersionInfo(hwndDlg);

            // PE properties
            PvpSetPeImageMachineType(hwndDlg);
            PvpSetPeImageTimeStamp(hwndDlg);
            PvpSetPeImageBaseAddress(hwndDlg);
            PvpSetPeImageEntryPoint(hwndDlg);
            PvpSetPeImageCheckSum(hwndDlg);
            PvpSetPeImageSubsystem(hwndDlg);
            PvpSetPeImageCharacteristics(hwndDlg);

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 80, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"VA");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Size");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 250, L"Characteristics");
            //PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"Hash");
            PhLoadListViewColumnsFromSetting(L"ImageGeneralListViewColumns", lvHandle);

            PvpSetPeImageSections(lvHandle);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageGeneralListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILE), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_NAME), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CHARACTERISTICS), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST), dialogItem, PH_ANCHOR_ALL);

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
                PhSetDialogItemText(hwndDlg, IDC_CHECKSUM, string->Buffer);
                PhDereferenceObject(string);
            }
            else if (headerCheckSum == realCheckSum)
            {
                string = PhFormatString(L"0x%Ix (correct)", headerCheckSum);
                PhSetDialogItemText(hwndDlg, IDC_CHECKSUM, string->Buffer);
                PhDereferenceObject(string);
            }
            else
            {
                string = PhFormatString(L"0x%Ix (incorrect, real 0x%Ix)", headerCheckSum, realCheckSum);
                PhSetDialogItemText(hwndDlg, IDC_CHECKSUM, string->Buffer);
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
                    PhSetDialogItemText(hwndDlg, IDC_COMPANYNAME_LINK, string->Buffer);
                    PhDereferenceObject(string);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_COMPANYNAME), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_COMPANYNAME_LINK), SW_SHOW);
                }
                else
                {
                    string = PhConcatStrings2(L"(Verified) ", PhGetStringOrEmpty(PvImageVersionInfo.CompanyName));
                    PhSetDialogItemText(hwndDlg, IDC_COMPANYNAME, string->Buffer);
                    PhDereferenceObject(string);
                }
            }
            else if (PvImageVerifyResult != VrUnknown)
            {
                string = PhConcatStrings2(L"(UNVERIFIED) ", PhGetStringOrEmpty(PvImageVersionInfo.CompanyName));
                PhSetDialogItemText(hwndDlg, IDC_COMPANYNAME, string->Buffer);
                PhDereferenceObject(string);
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_COMPANYNAME, PvpGetStringOrNa(PvImageVersionInfo.CompanyName));
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

BOOLEAN PvpLoadDbgHelp(
    _Inout_ PPH_SYMBOL_PROVIDER *SymbolProvider
    )
{
    static UNICODE_STRING symbolPathVarName = RTL_CONSTANT_STRING(L"_NT_SYMBOL_PATH");
    PPH_STRING symbolSearchPath;
    PPH_SYMBOL_PROVIDER symbolProvider;
    UNICODE_STRING symbolPathUs;
    WCHAR buffer[512];

    // Load symbol path from _NT_SYMBOL_PATH if configured by the user.
    RtlInitEmptyUnicodeString(&symbolPathUs, buffer, sizeof(buffer));

    if (NT_SUCCESS(RtlQueryEnvironmentVariable_U(NULL, &symbolPathVarName, &symbolPathUs)))
    {
        symbolSearchPath = PhCreateStringFromUnicodeString(&symbolPathUs);
    }
    else
    {
        // Set the default path (C:\\Symbols is the default hard-coded path for livekd). 
        symbolSearchPath = PhCreateString(L"SRV*C:\\Symbols*http://msdl.microsoft.com/download/symbols");
    }

    symbolProvider = PhCreateSymbolProvider(NULL);
    PhSetSearchPathSymbolProvider(symbolProvider, symbolSearchPath->Buffer);
    PhDereferenceObject(symbolSearchPath);

    *SymbolProvider = symbolProvider;
    return TRUE;
}

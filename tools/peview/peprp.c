/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010-2011 wj32
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
#include <workqueue.h>
#include <verify.h>
#include <shellapi.h>

#define PVM_CHECKSUM_DONE (WM_APP + 1)
#define PVM_VERIFY_DONE (WM_APP + 2)

INT_PTR CALLBACK PvpPeGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

typedef enum _PVP_IMAGE_GENERAL_CATEGORY
{
    PVP_IMAGE_GENERAL_CATEGORY_BASICINFO,
    PVP_IMAGE_GENERAL_CATEGORY_FILEINFO,
    PVP_IMAGE_GENERAL_CATEGORY_DEBUGINFO,
    PVP_IMAGE_GENERAL_CATEGORY_MAXIMUM
} PVP_IMAGE_GENERAL_CATEGORY;

typedef enum _PVP_IMAGE_GENERAL_INDEX
{
    PVP_IMAGE_GENERAL_INDEX_NAME,
    PVP_IMAGE_GENERAL_INDEX_TIMESTAMP,
    PVP_IMAGE_GENERAL_INDEX_IMAGEBASE,
    PVP_IMAGE_GENERAL_INDEX_IMAGESIZE,
    PVP_IMAGE_GENERAL_INDEX_ENTRYPOINT,
    PVP_IMAGE_GENERAL_INDEX_CHECKSUM,
    PVP_IMAGE_GENERAL_INDEX_CHECKSUMIAT,
    PVP_IMAGE_GENERAL_INDEX_SUBSYSTEM,
    PVP_IMAGE_GENERAL_INDEX_SUBSYSTEMVERSION,
    PVP_IMAGE_GENERAL_INDEX_CHARACTERISTICS,

    //PVP_IMAGE_GENERAL_INDEX_FILEATTRIBUTES,
    PVP_IMAGE_GENERAL_INDEX_FILECREATEDTIME,
    PVP_IMAGE_GENERAL_INDEX_FILEMODIFIEDTIME,
    PVP_IMAGE_GENERAL_INDEX_FILELASTWRITETIME,
    PVP_IMAGE_GENERAL_INDEX_FILEINDEX,
    PVP_IMAGE_GENERAL_INDEX_FILEID,
    PVP_IMAGE_GENERAL_INDEX_FILEOBJECTID,

    PVP_IMAGE_GENERAL_INDEX_DEBUGPDB,
    PVP_IMAGE_GENERAL_INDEX_DEBUGIMAGE,
    PVP_IMAGE_GENERAL_INDEX_DEBUGVCFEATURE,

    PVP_IMAGE_GENERAL_INDEX_MAXIMUM
} PVP_IMAGE_GENERAL_INDEX;

typedef struct _PVP_PE_GENERAL_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
    ULONG ListViewRowCache[PVP_IMAGE_GENERAL_INDEX_MAXIMUM];
} PVP_PE_GENERAL_CONTEXT, *PPVP_PE_GENERAL_CONTEXT;

typedef struct _IMAGE_DEBUG_REPRO_ENTRY
{
    ULONG Length;
    BYTE Buffer[1];
} IMAGE_DEBUG_REPRO_ENTRY, *PIMAGE_DEBUG_REPRO_ENTRY;

typedef struct _IMAGE_DEBUG_VC_FEATURE_ENTRY
{
    ULONG PreVCPlusPlusCount;
    ULONG CAndCPlusPlusCount;
    ULONG GuardStackCount;
    ULONG SdlCount;
    ULONG GuardCount;
} IMAGE_DEBUG_VC_FEATURE_ENTRY, *PIMAGE_DEBUG_VC_FEATURE_ENTRY;

PH_MAPPED_IMAGE PvMappedImage;
PIMAGE_COR20_HEADER PvImageCor20Header = NULL;
PPH_SYMBOL_PROVIDER PvSymbolProvider = NULL;
HICON PvImageSmallIcon = NULL;
HICON PvImageLargeIcon = NULL;
PH_IMAGE_VERSION_INFO PvImageVersionInfo;
static VERIFY_RESULT PvImageVerifyResult;
static PPH_STRING PvImageSignerName;

VOID PvPeProperties(
    VOID
    )
{
    PPV_PROPCONTEXT propContext;
    PH_MAPPED_IMAGE_IMPORTS imports;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PIMAGE_LOAD_CONFIG_DIRECTORY32 config32;
    PIMAGE_LOAD_CONFIG_DIRECTORY64 config64;
    PIMAGE_DATA_DIRECTORY entry;
    BOOLEAN hasEhContTable = FALSE;

    if (!PhExtractIcon(PvFileName->Buffer, &PvImageLargeIcon, &PvImageSmallIcon))
    {
        PhGetStockApplicationIcon(&PvImageSmallIcon, &PvImageLargeIcon);
    }

    if (PvpLoadDbgHelp(&PvSymbolProvider))
    {
        // Load current PE pdb
        // TODO: Move into seperate thread.

        if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            PhLoadModuleSymbolProvider(
                PvSymbolProvider,
                PvFileName->Buffer,
                (ULONG64)PvMappedImage.NtHeaders32->OptionalHeader.ImageBase,
                PvMappedImage.NtHeaders32->OptionalHeader.SizeOfImage
                );
        }
        else
        {
            PhLoadModuleSymbolProvider(
                PvSymbolProvider,
                PvFileName->Buffer,
                (ULONG64)PvMappedImage.NtHeaders->OptionalHeader.ImageBase,
                PvMappedImage.NtHeaders->OptionalHeader.SizeOfImage
                );
        }
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

        // Sections page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_PESECTIONS),
            PvPeSectionsDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        // Directories page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_PEDIRECTORY),
            PvpPeDirectoryDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

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

        // TLS page
        if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_TLS, &entry)) && entry->VirtualAddress)
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_TLS),
                PvpPeTlsDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // RICH header page
        {
            // .NET executables don't include a RICH header.
            if (!(NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &entry)) && entry->VirtualAddress))
            {
                newPage = PvCreatePropPageContext(
                    MAKEINTRESOURCE(IDD_PEPRODID),
                    PvpPeProdIdDlgProc,
                    NULL
                    );
                PvAddPropPage(propContext, newPage);
            }
        }

        // Certificates page
        if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_SECURITY, &entry)) && entry->VirtualAddress)
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PESECURITY),
                PvpPeSecurityDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // Debug page
        if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_DEBUG, &entry)) && entry->VirtualAddress)
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PEDEBUG),
                PvpPeDebugDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // EH continuation page
        {
            BOOLEAN has_ehcont = FALSE;

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                if (NT_SUCCESS(PhGetMappedImageLoadConfig32(&PvMappedImage, &config32)) &&
                    RTL_CONTAINS_FIELD(config32, config32->Size, GuardEHContinuationCount))
                {
                    if (config32->GuardEHContinuationTable && config32->GuardEHContinuationCount)
                        has_ehcont = TRUE;
                }
            }
            else
            {
                if (NT_SUCCESS(PhGetMappedImageLoadConfig64(&PvMappedImage, &config64)) &&
                    RTL_CONTAINS_FIELD(config64, config64->Size, GuardEHContinuationCount))
                {
                    if (config64->GuardEHContinuationTable && config64->GuardEHContinuationCount)
                        has_ehcont = TRUE;
                }
            }

            if (has_ehcont)
            {
                newPage = PvCreatePropPageContext(
                    MAKEINTRESOURCE(IDD_PEEHCONT),
                    PvpPeEhContDlgProc,
                    NULL
                    );
                PvAddPropPage(propContext, newPage);
            }
        }

        // Debug POGO page
        {
            BOOLEAN debugPogoValid = FALSE;
            PVOID debugEntry;

            if (NT_SUCCESS(PhGetMappedImageDebugEntryByType(
                &PvMappedImage,
                IMAGE_DEBUG_TYPE_POGO,
                NULL,
                &debugEntry
                )))
            {
                debugPogoValid = TRUE;
            }

            if (debugPogoValid)
            {
                newPage = PvCreatePropPageContext(
                    MAKEINTRESOURCE(IDD_PEDEBUGPOGO),
                    PvpPeDebugPogoDlgProc,
                    NULL
                    );
                PvAddPropPage(propContext, newPage);

                newPage = PvCreatePropPageContext(
                    MAKEINTRESOURCE(IDD_PEDEBUGCRT),
                    PvpPeDebugCrtDlgProc,
                    NULL
                    );
                PvAddPropPage(propContext, newPage);
            }
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

        // Links page
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PELINKS),
                PvpPeLinksDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // Processes page
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PIDS),
                PvpPeProcessesDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // Text preview page
        {
            newPage = PvCreatePropPageContext(
                MAKEINTRESOURCE(IDD_PEPREVIEW),
                PvpPePreviewDlgProc,
                NULL
                );
            PvAddPropPage(propContext, newPage);
        }

        // Symbols page
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
    HWND windowHandle = Parameter;
    PPH_STRING importHash = NULL;
    ULONG checkSum;
    HANDLE fileHandle;

    checkSum = PhCheckSumMappedImage(&PvMappedImage);

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ACCESS,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        0
        )))
    {
        BYTE importTableMd5Hash[16];

        if (NT_SUCCESS(RtlComputeImportTableHash(fileHandle, importTableMd5Hash, RTL_IMPORT_TABLE_HASH_REVISION)))
        {
            importHash = PhBufferToHexString(importTableMd5Hash, 16);
        }

        NtClose(fileHandle);
    }

    PostMessage(
        windowHandle,
        PVM_CHECKSUM_DONE,
        checkSum,
        (LPARAM)importHash
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

PPH_STRING PvpGetSectionCharacteristics(
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

    PhPrintPointer(pointer, UlongToPtr(Characteristics));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PvpSetPeImageVersionInfo(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING string;

    Static_SetIcon(GetDlgItem(WindowHandle, IDC_FILEICON), PvImageLargeIcon);

    if (PhGetIntegerSetting(L"EnableVersionSupport"))
        PhInitializeImageVersionInfo2(&PvImageVersionInfo, PvFileName->Buffer);
    else
        PhInitializeImageVersionInfo(&PvImageVersionInfo, PvFileName->Buffer);

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
    _In_ HWND ListViewHandle
    )
{
    ULONG machine;
    PWSTR type;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        machine = PvMappedImage.NtHeaders32->FileHeader.Machine;
    else
        machine = PvMappedImage.NtHeaders->FileHeader.Machine;

    switch (machine)
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

    PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_NAME, 1, type);
}

VOID PvpSetPeImageTimeStamp(
    _In_ HWND ListViewHandle
    )
{
    PIMAGE_DEBUG_REPRO_ENTRY debugEntry;
    ULONG debugEntryLength;
    LARGE_INTEGER time;
    SYSTEMTIME systemTime;
    PPH_STRING string = NULL;

    RtlSecondsSince1970ToTime(PvMappedImage.NtHeaders->FileHeader.TimeDateStamp, &time);
    PhLargeIntegerToLocalSystemTime(&systemTime, &time);

    if (NT_SUCCESS(PhGetMappedImageDebugEntryByType(
        &PvMappedImage,
        IMAGE_DEBUG_TYPE_REPRO,
        &debugEntryLength,
        &debugEntry
        )))
    {
        if (debugEntryLength > 0)
        {
            PPH_STRING timeStamp;

            __try
            {
                if (debugEntry->Length == debugEntryLength - sizeof(ULONG))
                    string = PhBufferToHexStringEx(debugEntry->Buffer, debugEntry->Length, FALSE);
                else
                    string = PhBufferToHexStringEx(debugEntry->Buffer, debugEntryLength - sizeof(ULONG), FALSE);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                string = PhGetStatusMessage(GetExceptionCode(), 0);
            }

            timeStamp = PhBufferToHexStringEx((PBYTE)&PvMappedImage.NtHeaders->FileHeader.TimeDateStamp, sizeof(ULONG), FALSE);

            if (PhEndsWithString(string, timeStamp, TRUE))
                PhMoveReference(&string, PhConcatStringRefZ(&string->sr, L" (deterministic)"));
            else
                PhMoveReference(&string, PhConcatStringRefZ(&string->sr, L" (deterministic) (incorrect)"));

            PhDereferenceObject(timeStamp);
        }
        else // CLR images
        {
            string = PhFormatDateTime(&systemTime);
            PhSwapReference(&string, PhFormatString(
                L"%s (%lx) (deterministic)",
                string->Buffer,
                PvMappedImage.NtHeaders->FileHeader.TimeDateStamp
                ));
        }
    }

    if (string)
    {
        PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_TIMESTAMP, 1, string->Buffer);
        PhDereferenceObject(string);
    }
    else
    {
        string = PhFormatDateTime(&systemTime);
        PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_TIMESTAMP, 1, string->Buffer);
        PhDereferenceObject(string);
    }
}

VOID PvpSetPeImageBaseAddress(
    _In_ HWND ListViewHandle
    )
{
    ULONGLONG imagebase;
    PPH_STRING string;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        imagebase = PvMappedImage.NtHeaders32->OptionalHeader.ImageBase;
    else
        imagebase = PvMappedImage.NtHeaders->OptionalHeader.ImageBase;

    string = PhFormatString(L"0x%I64x", imagebase);
    PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_IMAGEBASE, 1, string->Buffer);
    PhDereferenceObject(string);
}

VOID PvpSetPeImageSize(
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING string;
    ULONG lastRawDataAddress = 0;
    ULONG64 lastRawDataOffset = 0;
    ULONG64 lastRawDataAddressSize = 0;

    // https://reverseengineering.stackexchange.com/questions/2014/how-can-one-extract-the-appended-data-of-a-portable-executable/2015#2015

    for (ULONG i = 0; i < PvMappedImage.NumberOfSections; i++)
    {
        if (PvMappedImage.Sections[i].PointerToRawData > lastRawDataAddress)
        {
            lastRawDataAddress = PvMappedImage.Sections[i].PointerToRawData;
            lastRawDataOffset = (ULONG64)PTR_ADD_OFFSET(lastRawDataAddress, PvMappedImage.Sections[i].SizeOfRawData);
        }
    }

    if (PvMappedImage.Size != lastRawDataOffset)
    {
        BOOLEAN success = FALSE;
        PIMAGE_DATA_DIRECTORY dataDirectory;

        if (NT_SUCCESS(PhGetMappedImageDataEntry(
            &PvMappedImage,
            IMAGE_DIRECTORY_ENTRY_SECURITY,
            &dataDirectory
            )))
        {
            if (
                dataDirectory->VirtualAddress &&
                (lastRawDataOffset + dataDirectory->Size == PvMappedImage.Size) &&
                (lastRawDataOffset == dataDirectory->VirtualAddress)
                )
            {
                success = TRUE;
            }
        }

        if (success)
        {
            string = PhFormatString(L"%s", PhaFormatSize(PvMappedImage.Size, ULONG_MAX)->Buffer);
        }
        else
        {
            WCHAR pointer[PH_PTR_STR_LEN_1];

            PhPrintPointer(pointer, UlongToPtr(lastRawDataAddress));

            string = PhFormatString(
                L"%s (incorrect, %s) (%s - %s)",
                PhaFormatSize(lastRawDataOffset, ULONG_MAX)->Buffer,
                PhaFormatSize(PvMappedImage.Size, ULONG_MAX)->Buffer,
                pointer,
                PhaFormatSize(PvMappedImage.Size - lastRawDataOffset, ULONG_MAX)->Buffer
                );
        }
    }
    else
    {
        string = PhFormatString(L"%s", PhaFormatSize(PvMappedImage.Size, ULONG_MAX)->Buffer);
    }

    PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_IMAGESIZE, 1, string->Buffer);
    PhDereferenceObject(string);
}

VOID PvpSetPeImageEntryPoint(
    _In_ HWND ListViewHandle
    )
{
    ULONG addressOfEntryPoint;
    PPH_STRING string;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        addressOfEntryPoint = PvMappedImage.NtHeaders32->OptionalHeader.AddressOfEntryPoint;
    else
        addressOfEntryPoint = PvMappedImage.NtHeaders->OptionalHeader.AddressOfEntryPoint;

    string = PhFormatString(L"0x%I32x", addressOfEntryPoint);
    PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_ENTRYPOINT, 1, string->Buffer);
    PhDereferenceObject(string);
}

VOID PvpSetPeImageCheckSum(
    _In_ HWND WindowHandle,
    _In_ HWND ListViewHandle
    )
{
    PPH_STRING string;

    string = PhFormatString(L"0x%I32x (verifying...)", PvMappedImage.NtHeaders->OptionalHeader.CheckSum); // same for 32-bit and 64-bit images

    PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_CHECKSUM, 1, string->Buffer);
    PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_CHECKSUMIAT, 1, L"(verifying...)");

    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), CheckSumImageThreadStart, WindowHandle);

    PhDereferenceObject(string);
}

VOID PvpSetPeImageSubsystem(
    _In_ HWND ListViewHandle
    )
{
    ULONG subsystem;
    PWSTR type;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        subsystem = PvMappedImage.NtHeaders32->OptionalHeader.Subsystem;
    else
        subsystem = PvMappedImage.NtHeaders->OptionalHeader.Subsystem;

    switch (subsystem)
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

    PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_SUBSYSTEM, 1, type);
    PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_SUBSYSTEMVERSION, 1, PhaFormatString(
        L"%u.%u",
        PvMappedImage.NtHeaders->OptionalHeader.MajorSubsystemVersion, // same for 32-bit and 64-bit images
        PvMappedImage.NtHeaders->OptionalHeader.MinorSubsystemVersion
        )->Buffer);
}

VOID PvpSetPeImageCharacteristics(
    _In_ HWND ListViewHandle
    )
{
    ULONG characteristics;
    ULONG characteristicsDll;
    PH_STRING_BUILDER stringBuilder;
    ULONG debugEntryLength;
    PVOID debugEntry;
    //WCHAR pointer[PH_PTR_STR_LEN_1];

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        characteristics = PvMappedImage.NtHeaders32->FileHeader.Characteristics;
    else
        characteristics = PvMappedImage.NtHeaders->FileHeader.Characteristics;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        characteristicsDll = PvMappedImage.NtHeaders32->OptionalHeader.DllCharacteristics;
    else
        characteristicsDll = PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics;

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)
        PhAppendStringBuilder2(&stringBuilder, L"Executable, ");
    if (characteristics & IMAGE_FILE_DLL)
        PhAppendStringBuilder2(&stringBuilder, L"DLL, ");
    if (characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)
        PhAppendStringBuilder2(&stringBuilder, L"Large address aware, ");
    if (characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP)
        PhAppendStringBuilder2(&stringBuilder, L"Removable run from swap, ");
    if (characteristics & IMAGE_FILE_NET_RUN_FROM_SWAP)
        PhAppendStringBuilder2(&stringBuilder, L"Net run from swap, ");
    if (characteristics & IMAGE_FILE_SYSTEM)
        PhAppendStringBuilder2(&stringBuilder, L"System, ");
    if (characteristics & IMAGE_FILE_UP_SYSTEM_ONLY)
        PhAppendStringBuilder2(&stringBuilder, L"Uni-processor only, ");

    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA)
        PhAppendStringBuilder2(&stringBuilder, L"High entropy VA, ");
    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)
        PhAppendStringBuilder2(&stringBuilder, L"Dynamic base, ");
    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY)
        PhAppendStringBuilder2(&stringBuilder, L"Force integrity check, ");
    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_NX_COMPAT)
        PhAppendStringBuilder2(&stringBuilder, L"NX compatible, ");
    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_NO_ISOLATION)
        PhAppendStringBuilder2(&stringBuilder, L"No isolation, ");
    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_NO_SEH)
        PhAppendStringBuilder2(&stringBuilder, L"No SEH, ");
    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_NO_BIND)
        PhAppendStringBuilder2(&stringBuilder, L"Do not bind, ");
    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_APPCONTAINER)
        PhAppendStringBuilder2(&stringBuilder, L"AppContainer, ");
    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_WDM_DRIVER)
        PhAppendStringBuilder2(&stringBuilder, L"WDM driver, ");
    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_GUARD_CF)
        PhAppendStringBuilder2(&stringBuilder, L"Control Flow Guard, ");
    if (characteristicsDll & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE)
        PhAppendStringBuilder2(&stringBuilder, L"Terminal server aware, ");

    if (NT_SUCCESS(PhGetMappedImageDebugEntryByType(
        &PvMappedImage,
        IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS,
        &debugEntryLength,
        &debugEntry
        )))
    {
        ULONG characteristics = ULONG_MAX;

        if (debugEntryLength == sizeof(ULONG))
            characteristics = *(ULONG*)debugEntry;

        if (characteristics != ULONG_MAX)
        {
            if (characteristics & IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT)
                PhAppendStringBuilder2(&stringBuilder, L"CET compatible, ");
        }
    }

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    //PhPrintPointer(pointer, UlongToPtr(characteristics));
    //PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_CHARACTERISTICS, 1, PhFinalStringBuilderString(&stringBuilder)->Buffer);
    PhDeleteStringBuilder(&stringBuilder);
}

PPH_STRING PvGetRelativeTimeString(
    _In_ PLARGE_INTEGER Time
    )
{
    LARGE_INTEGER time;
    LARGE_INTEGER currentTime;
    SYSTEMTIME timeFields;
    PPH_STRING timeRelativeString;
    PPH_STRING timeString;

    time = *Time;
    PhQuerySystemTime(&currentTime);
    timeRelativeString = PH_AUTO(PhFormatTimeSpanRelative(currentTime.QuadPart - time.QuadPart));

    PhLargeIntegerToLocalSystemTime(&timeFields, &time);
    timeString = PhaFormatDateTime(&timeFields);

    return PhaFormatString(L"%s (%s ago)", timeString->Buffer, timeRelativeString->Buffer);
}

VOID PvpSetPeImageFileProperties(
    _In_ HWND ListViewHandle
    )
{
    HANDLE fileHandle;
    FILE_BASIC_INFORMATION fileInfo;
    FILE_INTERNAL_INFORMATION internalInfo;
    FILE_OBJECTID_BUFFER objectInfo;
    FILE_ID_INFORMATION fileIdInfo;
    IO_STATUS_BLOCK isb;

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        if (NT_SUCCESS(NtQueryInformationFile(
            fileHandle,
            &isb,
            &fileInfo,
            sizeof(FILE_BASIC_INFORMATION),
            FileBasicInformation
            )))
        {
            if (fileInfo.CreationTime.QuadPart != 0)
            {
                PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_FILECREATEDTIME, 1, PvGetRelativeTimeString(&fileInfo.CreationTime)->Buffer);
            }

            if (fileInfo.LastWriteTime.QuadPart != 0)
            {
                PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_FILEMODIFIEDTIME, 1, PvGetRelativeTimeString(&fileInfo.LastWriteTime)->Buffer);
            }

            if (fileInfo.ChangeTime.QuadPart != 0)
            {
                PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_FILELASTWRITETIME, 1, PvGetRelativeTimeString(&fileInfo.ChangeTime)->Buffer);
            }

            if (fileInfo.FileAttributes != 0)
            {
                //PH_STRING_BUILDER stringBuilder;
                //WCHAR pointer[PH_PTR_STR_LEN_1];
                //
                //PhInitializeStringBuilder(&stringBuilder, 0x100);
                //
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_READONLY)
                //    PhAppendStringBuilder2(&stringBuilder, L"Readonly, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_HIDDEN)
                //    PhAppendStringBuilder2(&stringBuilder, L"Hidden, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_SYSTEM)
                //    PhAppendStringBuilder2(&stringBuilder, L"System, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                //    PhAppendStringBuilder2(&stringBuilder, L"Directory, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_ARCHIVE)
                //    PhAppendStringBuilder2(&stringBuilder, L"Archive, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_DEVICE)
                //    PhAppendStringBuilder2(&stringBuilder, L"Device, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_NORMAL)
                //    PhAppendStringBuilder2(&stringBuilder, L"Normal, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_TEMPORARY)
                //    PhAppendStringBuilder2(&stringBuilder, L"Temporary, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE)
                //    PhAppendStringBuilder2(&stringBuilder, L"Sparse, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                //    PhAppendStringBuilder2(&stringBuilder, L"Reparse point, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_COMPRESSED)
                //    PhAppendStringBuilder2(&stringBuilder, L"Compressed, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_OFFLINE)
                //    PhAppendStringBuilder2(&stringBuilder, L"Offline, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
                //    PhAppendStringBuilder2(&stringBuilder, L"Not indexed, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
                //    PhAppendStringBuilder2(&stringBuilder, L"Encrypted, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM)
                //    PhAppendStringBuilder2(&stringBuilder, L"Integiry, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_VIRTUAL)
                //    PhAppendStringBuilder2(&stringBuilder, L"Vitual, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA)
                //    PhAppendStringBuilder2(&stringBuilder, L"No scrub, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_EA)
                //    PhAppendStringBuilder2(&stringBuilder, L"Extended attributes, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_PINNED)
                //    PhAppendStringBuilder2(&stringBuilder, L"Pinned, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_UNPINNED)
                //    PhAppendStringBuilder2(&stringBuilder, L"Unpinned, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_RECALL_ON_OPEN)
                //    PhAppendStringBuilder2(&stringBuilder, L"Recall on opened, ");
                //if (fileInfo.FileAttributes & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS)
                //    PhAppendStringBuilder2(&stringBuilder, L"Recall on data, ");
                //if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                //    PhRemoveEndStringBuilder(&stringBuilder, 2);

                //PhPrintPointer(pointer, UlongToPtr(fileInfo.FileAttributes));
                //PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

                //PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_FILEATTRIBUTES, 1, PhFinalStringBuilderString(&stringBuilder)->Buffer);
                //PhDeleteStringBuilder(&stringBuilder);
            }
        }

        if (NT_SUCCESS(NtQueryInformationFile(
            fileHandle,
            &isb,
            &internalInfo,
            sizeof(FILE_INTERNAL_INFORMATION),
            FileInternalInformation
            )))
        {
            PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_FILEINDEX, 1, PhaFormatUInt64(internalInfo.IndexNumber.QuadPart, FALSE)->Buffer);
        }

        if (NT_SUCCESS(NtQueryInformationFile(
            fileHandle,
            &isb,
            &fileIdInfo,
            sizeof(FILE_ID_INFORMATION),
            FileIdInformation
            )))
        {
            PPH_STRING string;

            string = PhFormatGuid((PGUID)fileIdInfo.FileId.Identifier);
            PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_FILEID, 1, string->Buffer);
            PhDereferenceObject(string);
        }

        if (NT_SUCCESS(NtFsControlFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            FSCTL_GET_OBJECT_ID,
            NULL,
            0,
            &objectInfo,
            sizeof(objectInfo)
            )))
        {
            PPH_STRING string;
            PGUID guid = (PGUID)objectInfo.ObjectId;

            // The swap returns the same value as 'fsutil objectid query filepath' (dmex)
            guid->Data1 = _byteswap_ulong(guid->Data1);
            guid->Data2 = _byteswap_ushort(guid->Data2);
            guid->Data3 = _byteswap_ushort(guid->Data3);
            //TODO: highlight IsEqualGUID(objectInfo.ObjectId, objectInfo.BirthObjectId)

            string = PhFormatGuid(guid);
            PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_FILEOBJECTID, 1, string->Buffer);
            PhDereferenceObject(string);
        }

        NtClose(fileHandle);
    }
}

VOID PvpSetPeImageDebugPdb(
    _In_ HWND ListViewHandle
    )
{
    PCODEVIEW_INFO_PDB70 codeviewEntry;
    ULONG debugEntryLength;

    if (NT_SUCCESS(PhGetMappedImageDebugEntryByType(
        &PvMappedImage,
        IMAGE_DEBUG_TYPE_CODEVIEW,
        &debugEntryLength,
        &codeviewEntry
        )))
    {
        PPH_STRING string;

        //if (debugEntryLength == sizeof(IMAGE_DEBUG_DIRECTORY_CODEVIEW))
        if (codeviewEntry->Signature == CODEVIEW_SIGNATURE_RSDS)
        {
            string = PhFormatGuid(&codeviewEntry->PdbGuid);
            PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_DEBUGPDB, 1, string->Buffer);
            PhDereferenceObject(string);

            string = PhConvertUtf8ToUtf16(codeviewEntry->ImageName);
            PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_DEBUGIMAGE, 1, string->Buffer);
            PhDereferenceObject(string);
        }
    }
}

VOID PvpSetPeImageDebugVCFeatures(
    _In_ HWND ListViewHandle
    )
{
    PIMAGE_DEBUG_VC_FEATURE_ENTRY debugEntry;
    ULONG debugEntryLength;

    if (NT_SUCCESS(PhGetMappedImageDebugEntryByType(
        &PvMappedImage,
        IMAGE_DEBUG_TYPE_VC_FEATURE,
        &debugEntryLength,
        &debugEntry
        )))
    {
        PPH_STRING vcfeatures;

        if (debugEntryLength != sizeof(IMAGE_DEBUG_VC_FEATURE_ENTRY))
            return;

        // Use the same format as dumpbin (dmex)
        vcfeatures = PhFormatString(
            L"C/C++ (%lu), GS (%lu), sdl (%lu), guardN (%lu), Pre-VC++ 11.00 (%lu)",
            debugEntry->CAndCPlusPlusCount,
            debugEntry->GuardStackCount,
            debugEntry->SdlCount,
            debugEntry->GuardCount,
            debugEntry->PreVCPlusPlusCount
            );

        PhSetListViewSubItem(ListViewHandle, PVP_IMAGE_GENERAL_INDEX_DEBUGVCFEATURE, 1, vcfeatures->Buffer);
        PhDereferenceObject(vcfeatures);
    }
}

VOID PvpSetPeImageProperties(
    _In_ PPVP_PE_GENERAL_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    ListView_EnableGroupView(Context->ListViewHandle, TRUE);
    PhAddListViewGroup(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, L"Image information");
    PhAddListViewGroup(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_FILEINFO, L"File information");
    PhAddListViewGroup(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_DEBUGINFO, L"Debug information");

    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, PVP_IMAGE_GENERAL_INDEX_NAME, L"Target machine", NULL);  
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, PVP_IMAGE_GENERAL_INDEX_TIMESTAMP, L"Time stamp", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, PVP_IMAGE_GENERAL_INDEX_IMAGEBASE, L"Image base", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, PVP_IMAGE_GENERAL_INDEX_IMAGESIZE, L"Image size", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, PVP_IMAGE_GENERAL_INDEX_ENTRYPOINT, L"Entry point", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, PVP_IMAGE_GENERAL_INDEX_CHECKSUM, L"Header checksum", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, PVP_IMAGE_GENERAL_INDEX_CHECKSUMIAT, L"Import checksum", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, PVP_IMAGE_GENERAL_INDEX_SUBSYSTEM, L"Subsystem", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, PVP_IMAGE_GENERAL_INDEX_SUBSYSTEMVERSION, L"Subsystem version", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_BASICINFO, PVP_IMAGE_GENERAL_INDEX_CHARACTERISTICS, L"Characteristics", NULL);
    //PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_FILEINFO, PVP_IMAGE_GENERAL_INDEX_FILEATTRIBUTES, L"Attributes", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_FILEINFO, PVP_IMAGE_GENERAL_INDEX_FILECREATEDTIME, L"Created time", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_FILEINFO, PVP_IMAGE_GENERAL_INDEX_FILEMODIFIEDTIME, L"Modified time", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_FILEINFO, PVP_IMAGE_GENERAL_INDEX_FILELASTWRITETIME, L"Updated time", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_FILEINFO, PVP_IMAGE_GENERAL_INDEX_FILEINDEX, L"File index", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_FILEINFO, PVP_IMAGE_GENERAL_INDEX_FILEID, L"File identifier", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_FILEINFO, PVP_IMAGE_GENERAL_INDEX_FILEOBJECTID, L"File object identifier", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_DEBUGINFO, PVP_IMAGE_GENERAL_INDEX_DEBUGPDB, L"Guid", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_DEBUGINFO, PVP_IMAGE_GENERAL_INDEX_DEBUGIMAGE, L"Image name", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_GENERAL_CATEGORY_DEBUGINFO, PVP_IMAGE_GENERAL_INDEX_DEBUGVCFEATURE, L"Feature count", NULL);

    PvpSetPeImageMachineType(Context->ListViewHandle);
    PvpSetPeImageTimeStamp(Context->ListViewHandle);
    PvpSetPeImageBaseAddress(Context->ListViewHandle);
    PvpSetPeImageSize(Context->ListViewHandle);
    PvpSetPeImageEntryPoint(Context->ListViewHandle);
    PvpSetPeImageCheckSum(Context->WindowHandle, Context->ListViewHandle);
    PvpSetPeImageSubsystem(Context->ListViewHandle);
    PvpSetPeImageCharacteristics(Context->ListViewHandle);
    // File information
    PvpSetPeImageFileProperties(Context->ListViewHandle);
    // Debug information
    PvpSetPeImageDebugPdb(Context->ListViewHandle);
    PvpSetPeImageDebugVCFeatures(Context->ListViewHandle);

    //ExtendedListView_SortItems(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

NTSTATUS PhpOpenFileSecurity(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    FILE_NETWORK_OPEN_INFORMATION networkOpenInfo;

    status = PhQueryFullAttributesFileWin32(PhGetString(PvFileName), &networkOpenInfo);

    if (!NT_SUCCESS(status))
        return status;

    if (networkOpenInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        status = PhCreateFileWin32(
            Handle,
            PhGetString(PvFileName),
            DesiredAccess | READ_CONTROL | WRITE_DAC | SYNCHRONIZE,
            FILE_ATTRIBUTE_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );
    }
    else
    {
        status = PhCreateFileWin32(
            Handle,
            PhGetString(PvFileName),
            DesiredAccess | READ_CONTROL | WRITE_DAC | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (!NT_SUCCESS(status))
        {
            status = PhCreateFileWin32(
                Handle,
                PhGetString(PvFileName),
                DesiredAccess | READ_CONTROL | SYNCHRONIZE,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );
        }
    }

    return status;
}

static COLORREF NTAPI PvpPeCharacteristicsColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    )
{
    PIMAGE_SECTION_HEADER imageSection = Param;

    if (imageSection->Characteristics & IMAGE_SCN_MEM_WRITE)
    {
        return RGB(0xf0, 0xa0, 0xa0);
    }
    if (imageSection->Characteristics & IMAGE_SCN_MEM_EXECUTE)
    {
        return RGB(0xf0, 0xb0, 0xb0);
    }
    if (imageSection->Characteristics & IMAGE_SCN_CNT_CODE)
    {
        return RGB(0xe0, 0xf0, 0xe0);
    }
    if (imageSection->Characteristics & IMAGE_SCN_MEM_READ)
    {
        return RGB(0xc0, 0xf0, 0xc0);
    }

    return RGB(0xff, 0xff, 0xff);
}

static INT NTAPI PvpPeVirtualAddressCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PIMAGE_SECTION_HEADER entry1 = Item1;
    PIMAGE_SECTION_HEADER entry2 = Item2;

    return uintcmp(entry1->VirtualAddress, entry2->VirtualAddress);
}

static INT NTAPI PvpPeSizeOfRawDataCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PIMAGE_SECTION_HEADER entry1 = Item1;
    PIMAGE_SECTION_HEADER entry2 = Item2;

    return uintcmp(entry1->SizeOfRawData, entry2->SizeOfRawData);
}

static INT NTAPI PvpPeCharacteristicsCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PIMAGE_SECTION_HEADER entry1 = Item1;
    PIMAGE_SECTION_HEADER entry2 = Item2;

    return uintcmp(entry1->Characteristics, entry2->Characteristics);
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
    PPVP_PE_GENERAL_CONTEXT context = NULL;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    if (uMsg == WM_INITDIALOG)
    {
        context = propPageContext->Context = PhAllocate(sizeof(PVP_PE_GENERAL_CONTEXT));
        memset(context, 0, sizeof(PVP_PE_GENERAL_CONTEXT));
    }
    else
    {
        context = propPageContext->Context;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 150, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 300, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            //PhLoadListViewColumnsFromSetting(L"ImageGeneralPropertiesListViewColumns", context->ListViewHandle);
            //PhLoadListViewSortColumnsFromSetting(L"ImageGeneralPropertiesListViewSort", context->ListViewHandle);

            if (context->ListViewImageList = ImageList_Create(2, 20, ILC_MASK | ILC_COLOR, 1, 1))
                ListView_SetImageList(context->ListViewHandle, context->ListViewImageList, LVSIL_SMALL);

            PvpSetPeImageVersionInfo(hwndDlg);
            PvpSetPeImageProperties(context);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewSortColumnsToSetting(L"ImageGeneralPropertiesListViewSort", context->ListViewHandle);
            PhSaveListViewColumnsToSetting(L"ImageGeneralPropertiesListViewColumns", context->ListViewHandle);

            if (context->ListViewImageList)
                ImageList_Destroy(context->ListViewImageList);

            PhFree(context);
            context = NULL;
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILE), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);

                PvDoPropPageLayout(hwndDlg);

                ExtendedListView_SetColumnWidth(context->ListViewHandle, 1, ELVSCW_AUTOSIZE_REMAININGSPACE);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 1, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case PVM_CHECKSUM_DONE:
        {
            PPH_STRING string;
            PPH_STRING importTableHash;
            ULONG headerCheckSum;
            ULONG realCheckSum;

            headerCheckSum = PvMappedImage.NtHeaders->OptionalHeader.CheckSum; // same for 32-bit and 64-bit images
            realCheckSum = (ULONG)wParam;
            importTableHash = (PPH_STRING)lParam;

            if (headerCheckSum == 0)
            {
                // Some executables, like .NET ones, don't have a check sum.
                string = PhFormatString(L"0x0 (real 0x%I32x)", realCheckSum);
                PhSetListViewSubItem(context->ListViewHandle, PVP_IMAGE_GENERAL_INDEX_CHECKSUM, 1, string->Buffer);
                PhDereferenceObject(string);
            }
            else if (headerCheckSum == realCheckSum)
            {
                string = PhFormatString(L"0x%I32x", headerCheckSum);
                PhSetListViewSubItem(context->ListViewHandle, PVP_IMAGE_GENERAL_INDEX_CHECKSUM, 1, string->Buffer);
                PhDereferenceObject(string);
            }
            else
            {
                string = PhFormatString(L"0x%I32x (incorrect, real 0x%I32x)", headerCheckSum, realCheckSum);
                PhSetListViewSubItem(context->ListViewHandle, PVP_IMAGE_GENERAL_INDEX_CHECKSUM, 1, string->Buffer);
                PhDereferenceObject(string);
            }

            if (importTableHash)
            {
                PhSetListViewSubItem(context->ListViewHandle, PVP_IMAGE_GENERAL_INDEX_CHECKSUMIAT, 1, PhGetStringOrEmpty(importTableHash));
                PhDereferenceObject(importTableHash);
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

            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    }

    if (context)
    {
        REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);
    }

    return FALSE;
}

BOOLEAN PvpLoadDbgHelp(
    _Inout_ PPH_SYMBOL_PROVIDER *SymbolProvider
    )
{
    PPH_STRING symbolSearchPath;
    PPH_SYMBOL_PROVIDER symbolProvider;

    symbolProvider = PhCreateSymbolProvider(NULL);

    if (symbolSearchPath = PhGetStringSetting(L"DbgHelpSearchPath"))
    {
        PhSetSearchPathSymbolProvider(symbolProvider, symbolSearchPath->Buffer);
        PhDereferenceObject(symbolSearchPath);
    }

    *SymbolProvider = symbolProvider;
    return TRUE;
}

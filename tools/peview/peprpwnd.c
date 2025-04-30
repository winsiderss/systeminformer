/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021
 *
 */

#include <peview.h>
#include <secedit.h>

typedef struct _PV_WINDOW_SECTION
{
    PH_STRINGREF Name;

    PVOID Instance;
    PWSTR Template;
    DLGPROC DialogProc;
    PVOID Parameter;

    HWND DialogHandle;
    HTREEITEM TreeItemHandle;
} PV_WINDOW_SECTION, *PPV_WINDOW_SECTION;

INT_PTR CALLBACK PvTabWindowDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#define SWP_NO_ACTIVATE_MOVE_SIZE_ZORDER (SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER)
#define SWP_SHOWWINDOW_ONLY (SWP_NO_ACTIVATE_MOVE_SIZE_ZORDER | SWP_SHOWWINDOW)
#define SWP_HIDEWINDOW_ONLY (SWP_NO_ACTIVATE_MOVE_SIZE_ZORDER | SWP_HIDEWINDOW)

VOID PvDestroyTabSection(
    _In_ PPV_WINDOW_SECTION Section
    );

VOID PvEnterTabSectionView(
    _In_ PPV_WINDOW_SECTION NewSection
    );

VOID PvLayoutTabSectionView(
    VOID
    );

VOID PvEnterTabSectionViewInner(
    _In_ PPV_WINDOW_SECTION Section,
    _Inout_ HDWP *ContainerDeferHandle
    );

VOID PvCreateTabSectionDialog(
    _In_ PPV_WINDOW_SECTION Section
    );

VOID PvTabWindowOnSize(
    VOID
    );

PPV_WINDOW_SECTION PvFindTabSectionByName(
    _In_ PPH_STRINGREF Name
    );

PPV_WINDOW_SECTION PvGetSelectedTabSection(
    _In_opt_ PVOID TreeItemHandle
    );

PPV_WINDOW_SECTION PvCreateTabSection(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    );

static HWND PvPropertiesWindowHandle = NULL;
static HWND PvTabTreeControl = NULL;
static HWND PvTabContainerControl = NULL;
static INT PvPropertiesWindowShowCommand = SW_SHOW;
static PH_LAYOUT_MANAGER PvTabWindowLayoutManager;
static PPH_LIST PvTabSectionList = NULL;
static PPV_WINDOW_SECTION PvTabCurrentSection = NULL;

VOID PvShowPePropertiesWindow(
    VOID
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    PvPropertiesWindowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_TABWINDOW),
        NULL,
        PvTabWindowDialogProc,
        NULL
        );

    if (PhGetIntegerSetting(L"MainWindowState") == SW_MAXIMIZE)
        PvPropertiesWindowShowCommand = SW_MAXIMIZE;

    ShowWindow(PvPropertiesWindowHandle, PvPropertiesWindowShowCommand);
    SetForegroundWindow(PvPropertiesWindowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(PvPropertiesWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
}

VOID PvAddTreeViewSections(
    VOID
    )
{
    PPV_WINDOW_SECTION section;
    PH_MAPPED_IMAGE_IMPORTS imports;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PIMAGE_LOAD_CONFIG_DIRECTORY32 config32;
    PIMAGE_LOAD_CONFIG_DIRECTORY64 config64;
    PIMAGE_DATA_DIRECTORY entry;

    PvTabSectionList = PhCreateList(30);
    PvTabCurrentSection = NULL;

    // General page
    section = PvCreateTabSection(
        L"General",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEGENERAL),
        PvPeGeneralDlgProc,
        NULL
        );

    // Headers page
    PvCreateTabSection(
        L"Headers",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEHEADERS),
        PvPeHeadersDlgProc,
        NULL
        );

    // Load Config page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &entry)))
    {
        PvCreateTabSection(
            L"Load Config",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PELOADCONFIG),
            PvPeLoadConfigDlgProc,
            NULL
            );
    }

    // Sections page
    PvCreateTabSection(
        L"Sections",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PESECTIONS),
        PvPeSectionsDlgProc,
        NULL
        );

    // Directories page
    PvCreateTabSection(
        L"Directories",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEDIRECTORY),
        PvPeDirectoryDlgProc,
        NULL
        );

    // Imports page
    if ((NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)) && imports.NumberOfDlls != 0) ||
        (NT_SUCCESS(PhGetMappedImageDelayImports(&imports, &PvMappedImage)) && imports.NumberOfDlls != 0))
    {
        PvCreateTabSection(
            L"Imports",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEIMPORTS),
            PvPeImportsDlgProc,
            NULL
            );
    }

    // Exports page
    if (NT_SUCCESS(PhGetMappedImageExports(&exports, &PvMappedImage)) && exports.NumberOfEntries != 0)
    {
        PPV_EXPORTS_PAGECONTEXT exportsPageContext;
        PPV_PROPPAGECONTEXT propPageContext;
        LPPROPSHEETPAGE propSheetPage;

        exportsPageContext = PhAllocateZero(sizeof(PV_EXPORTS_PAGECONTEXT));
        exportsPageContext->FreePropPageContext = TRUE;
        exportsPageContext->Context = ULongToPtr(0); // PhGetMappedImageExportsEx with no flags

        propPageContext = PhAllocateZero(sizeof(PV_PROPPAGECONTEXT));
        propPageContext->Context = exportsPageContext;
        propSheetPage = PhAllocateZero(sizeof(PROPSHEETPAGE));
        propSheetPage->lParam = (LPARAM)propPageContext;

        PvCreateTabSection(
            L"Exports",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEEXPORTS),
            PvPeExportsDlgProc,
            propSheetPage
            );
    }

    // Exports ARM64X page
    if (NT_SUCCESS(PhGetMappedImageExportsEx(&exports, &PvMappedImage, PH_GET_IMAGE_EXPORTS_ARM64X)) && exports.NumberOfEntries != 0)
    {
        PPV_EXPORTS_PAGECONTEXT exportsPageContext;
        PPV_PROPPAGECONTEXT propPageContext;
        LPPROPSHEETPAGE propSheetPage;

        exportsPageContext = PhAllocateZero(sizeof(PV_EXPORTS_PAGECONTEXT));
        exportsPageContext->FreePropPageContext = TRUE;
        exportsPageContext->Context = ULongToPtr(PH_GET_IMAGE_EXPORTS_ARM64X);

        propPageContext = PhAllocateZero(sizeof(PV_PROPPAGECONTEXT));
        propPageContext->Context = exportsPageContext;
        propSheetPage = PhAllocateZero(sizeof(PROPSHEETPAGE));
        propSheetPage->lParam = (LPARAM)propPageContext;

        PvCreateTabSection(
            L"Exports ARM64X",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEEXPORTS),
            PvPeExportsDlgProc,
            propSheetPage
            );
    }

    // Resources page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_RESOURCE, &entry)))
    {
        PvCreateTabSection(
            L"Resources",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PERESOURCES),
            PvPeResourcesDlgProc,
            NULL
            );
    }

    // CLR page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &entry)) &&
        (PvImageCor20Header = PhMappedImageRvaToVa(&PvMappedImage, entry->VirtualAddress, NULL)))
    {
        NTSTATUS status = STATUS_SUCCESS;

        __try
        {
            PhProbeAddress(
                PvImageCor20Header,
                sizeof(IMAGE_COR20_HEADER),
                PvMappedImage.ViewBase,
                PvMappedImage.ViewSize,
                4
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }

        if (NT_SUCCESS(status))
        {
            PvCreateTabSection(
                L"CLR",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PECLR),
                PvpPeClrDlgProc,
                NULL
                );

            PvCreateTabSection(
                L"CLR Imports",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PECLRIMPORTS),
                PvpPeClrImportsDlgProc,
                NULL
                );

            PvCreateTabSection(
                L"CLR Tables",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PECLRTABLES),
                PvpPeClrTablesDlgProc,
                NULL
                );
        }
    }

    // CFG page
    if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_GUARD_CF)
    {
        PvCreateTabSection(
            L"CFG",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PECFG),
            PvpPeCgfDlgProc,
            NULL
            );
    }

    // TLS page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_TLS, &entry)))
    {
        PvCreateTabSection(
            L"TLS",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_TLS),
            PvpPeTlsDlgProc,
            NULL
            );
    }

    // ProdId page
    {
        ULONG imageDosStubLength = ((PIMAGE_DOS_HEADER)PvMappedImage.ViewBase)->e_lfanew - RTL_SIZEOF_THROUGH_FIELD(IMAGE_DOS_HEADER, e_lfanew);

        if (imageDosStubLength != 0 && imageDosStubLength != 64)
        {
            PvCreateTabSection(
                L"ProdID",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEPRODID),
                PvpPeProdIdDlgProc,
                NULL
                );
        }
    }

    {
        BOOLEAN hasExceptions = FALSE;
        BOOLEAN hasExceptionsArm64X = FALSE;

        if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            if (NT_SUCCESS(PhGetMappedImageLoadConfig32(&PvMappedImage, &config32)) &&
                RTL_CONTAINS_FIELD(config32, config32->Size, SEHandlerCount))
            {
                if (config32->SEHandlerCount && config32->SEHandlerTable)
                    hasExceptions = TRUE;
            }
        }
        else
        {
            if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_EXCEPTION, &entry)))
            {
                IMAGE_DATA_DIRECTORY entryArm64X;

                hasExceptions = TRUE;

                if (NT_SUCCESS(PhRelocateMappedImageDataEntryARM64X(&PvMappedImage, entry, &entryArm64X)))
                    hasExceptionsArm64X = TRUE;
            }
        }

        // Exceptions page
        if (hasExceptions)
        {
            PPV_EXCEPTIONS_PAGECONTEXT exceptionsPageContext;
            PPV_PROPPAGECONTEXT propPageContext;
            LPPROPSHEETPAGE propSheetPage;

            exceptionsPageContext = PhAllocateZero(sizeof(PV_EXCEPTIONS_PAGECONTEXT));
            exceptionsPageContext->FreePropPageContext = TRUE;
            exceptionsPageContext->Context = ULongToPtr(0); // PhGetMappedImageExceptionsEx with no flags

            propPageContext = PhAllocateZero(sizeof(PV_PROPPAGECONTEXT));
            propPageContext->Context = exceptionsPageContext;
            propSheetPage = PhAllocateZero(sizeof(PROPSHEETPAGE));
            propSheetPage->lParam = (LPARAM)propPageContext;

            PvCreateTabSection(
                L"Exceptions",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEEXCEPTIONS),
                PvpPeExceptionDlgProc,
                propSheetPage
                );
        }

        // Exceptions ARM64X page
        if (hasExceptionsArm64X)
        {
            PPV_EXCEPTIONS_PAGECONTEXT exceptionsPageContext;
            PPV_PROPPAGECONTEXT propPageContext;
            LPPROPSHEETPAGE propSheetPage;

            exceptionsPageContext = PhAllocateZero(sizeof(PV_EXCEPTIONS_PAGECONTEXT));
            exceptionsPageContext->FreePropPageContext = TRUE;
            exceptionsPageContext->Context = ULongToPtr(PH_GET_IMAGE_EXCEPTIONS_ARM64X);

            propPageContext = PhAllocateZero(sizeof(PV_PROPPAGECONTEXT));
            propPageContext->Context = exceptionsPageContext;
            propSheetPage = PhAllocateZero(sizeof(PROPSHEETPAGE));
            propSheetPage->lParam = (LPARAM)propPageContext;

            PvCreateTabSection(
                L"Exceptions ARM64X",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEEXCEPTIONS),
                PvpPeExceptionDlgProc,
                propSheetPage
                );
        }
    }

    // Relocations page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_BASERELOC, &entry)))
    {
        PvCreateTabSection(
            L"Relocations",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PERELOCATIONS),
            PvpPeRelocationDlgProc,
            NULL
            );
    }

    // Dynmic Relocations page
    if (NT_SUCCESS(PhGetMappedImageDynamicRelocationsTable(&PvMappedImage, NULL)))
    {
        PvCreateTabSection(
            L"Dynamic Relocations",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PERELOCATIONS),
            PvpPeDynamicRelocationDlgProc,
            NULL
            );
    }

    // Hybrid Metadata page
    if (PhGetMappedImageCHPEVersion(&PvMappedImage))
    {
        PvCreateTabSection(
            L"Hybrid Metadata",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PELOADCONFIG),
            PvpPeCHPEDlgProc,
            NULL
            );
    }

    // Certificates page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_SECURITY, &entry)))
    {
        PvCreateTabSection(
            L"Certificates",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PESECURITY),
            PvpPeSecurityDlgProc,
            NULL
            );
    }

    // Debug page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_DEBUG, &entry)))
    {
        PvCreateTabSection(
            L"Debug",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEDEBUG),
            PvpPeDebugDlgProc,
            NULL
            );
    }

    // Volatile page
    {
        BOOLEAN valid = FALSE;

        if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            if (NT_SUCCESS(PhGetMappedImageLoadConfig32(&PvMappedImage, &config32)) &&
                RTL_CONTAINS_FIELD(config32, config32->Size, VolatileMetadataPointer))
            {
                if (config32->VolatileMetadataPointer)
                    valid = TRUE;
            }
        }
        else
        {
            if (NT_SUCCESS(PhGetMappedImageLoadConfig64(&PvMappedImage, &config64)) &&
                RTL_CONTAINS_FIELD(config64, config64->Size, VolatileMetadataPointer))
            {
                if (config64->VolatileMetadataPointer)
                    valid = TRUE;
            }
        }

        if (valid)
        {
            PvCreateTabSection(
                L"Volatile Metadata",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEVOLATILE),
                PvpPeVolatileDlgProc,
                NULL
                );
        }
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
            PvCreateTabSection(
                L"EH Continuation",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEEHCONT),
                PvpPeEhContDlgProc,
                NULL
                );
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
            PvCreateTabSection(
                L"POGO",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEDEBUGPOGO),
                PvpPeDebugPogoDlgProc,
                NULL
                );

            PvCreateTabSection(
                L"CRT",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEDEBUGCRT),
                PvpPeDebugCrtDlgProc,
                NULL
                );
        }
    }

    // Properties page
    PvCreateTabSection(
        L"Properties",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEPROPSTORAGE),
        PvpPePropStoreDlgProc,
        NULL
        );

    // Extended attributes page
    PvCreateTabSection(
        L"Attributes",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEATTR),
        PvpPeExtendedAttributesDlgProc,
        NULL
        );

    // Streams page
    PvCreateTabSection(
        L"Streams",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PESTREAMS),
        PvpPeStreamsDlgProc,
        NULL
        );

    // Layout page
    PvCreateTabSection(
        L"Layout",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PELAYOUT),
        PvpPeLayoutDlgProc,
        NULL
        );

    // Links page
    PvCreateTabSection(
        L"Links",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PELINKS),
        PvpPeLinksDlgProc,
        NULL
        );

    // Processes page
    PvCreateTabSection(
        L"Processes",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PIDS),
        PvpPeProcessesDlgProc,
        NULL
        );

    // Hashes page
    PvCreateTabSection(
        L"Hashes",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEHASHES),
        PvpPeHashesDlgProc,
        NULL
        );

    // Text preview page
    PvCreateTabSection(
        L"Preview",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEPREVIEW),
        PvpPePreviewDlgProc,
        NULL
        );

    // Symbols page
    PvCreateTabSection(
        L"Symbols",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PESYMBOLS),
        PvpSymbolsDlgProc,
        NULL
        );

    // Strings page
    PvCreateTabSection(
        L"Strings",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_STRINGS),
        PvStringsDlgProc,
        NULL
        );

    // VS_VERSIONINFO page
    PvCreateTabSection(
        L"Version",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEVERSIONINFO),
        PvpPeVersionInfoDlgProc,
        NULL
        );

    // Mappings page
    if (KphLevelEx(FALSE) >= KphLevelMed)
    {
        PvCreateTabSection(
            L"Mappings",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PERELOCATIONS),
            PvpMappingsDlgProc,
            NULL
            );
    }

    // MUI page
    PvCreateTabSection(
        L"MUI",
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEVERSIONINFO),
        PvpPeMuiResourceDlgProc,
        NULL
        );

    if (PhGetIntegerSetting(L"MainWindowPageRestoreEnabled"))
    {
        PPH_STRING startPage;
        PPV_WINDOW_SECTION startSection;
        BOOLEAN foundStartPage = FALSE;

        if (startPage = PhGetStringSetting(L"MainWindowPage"))
        {
            if (startSection = PvFindTabSectionByName(&startPage->sr))
            {
                TreeView_SelectItem(PvTabTreeControl, startSection->TreeItemHandle);
                foundStartPage = TRUE;
            }

            PhDereferenceObject(startPage);
        }

        if (!foundStartPage)
        {
            TreeView_SelectItem(PvTabTreeControl, section->TreeItemHandle);
        }

        SetFocus(PvTabTreeControl);
    }
    else
    {
        TreeView_SelectItem(PvTabTreeControl, section->TreeItemHandle);
        SetFocus(PvTabTreeControl);
        //PvEnterTabSectionView(section);
    }

    PvTabWindowOnSize();
}

INT_PTR CALLBACK PvTabWindowDialogProc(
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
            PvTabTreeControl = GetDlgItem(hwndDlg, IDC_SECTIONTREE);
            PvTabContainerControl = GetDlgItem(hwndDlg, IDD_CONTAINER);

            PhSetWindowText(hwndDlg, PhaFormatString(L"%s Properties", PhGetString(PvFileName))->Buffer);

            //PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_SEPARATOR), SS_OWNERDRAW, SS_OWNERDRAW);
            PhSetControlTheme(PvTabTreeControl, L"explorer");
            TreeView_SetExtendedStyle(PvTabTreeControl, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
            TreeView_SetBkColor(PvTabTreeControl, GetSysColor(COLOR_3DFACE));
            PvSetTreeViewImageList(hwndDlg, PvTabTreeControl);

            PhInitializeLayoutManager(&PvTabWindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&PvTabWindowLayoutManager, PvTabTreeControl, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            //PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDC_SEPARATOR), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&PvTabWindowLayoutManager, PvTabContainerControl, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDC_OPTIONS), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDC_SECURITY), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhInitializeWindowTheme(hwndDlg);

            {
                HICON smallIcon;
                HICON largeIcon;

                if (!PhExtractIcon(PvFileName->Buffer, &PvImageLargeIcon, &PvImageSmallIcon))
                {
                    PhGetStockApplicationIcon(&PvImageSmallIcon, &PvImageLargeIcon);
                }

                PhGetStockApplicationIcon(&smallIcon, &largeIcon);

                SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
                SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)largeIcon);
            }

            if (PvpLoadDbgHelp(&PvSymbolProvider))
            {
                PPH_STRING fileName;

                if (NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), PvMappedImage.ViewBase, &fileName)))
                {
                    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                    {
                        PhLoadModuleSymbolProvider(
                            PvSymbolProvider,
                            fileName,
                            PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, 0),
                            PvMappedImage.NtHeaders32->OptionalHeader.SizeOfImage
                            );
                    }
                    else
                    {
                        PhLoadModuleSymbolProvider(
                            PvSymbolProvider,
                            fileName,
                            PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, 0),
                            PvMappedImage.NtHeaders->OptionalHeader.SizeOfImage
                            );
                    }

                    PhDereferenceObject(fileName);
                }

                PhLoadModulesForVirtualSymbolProvider(PvSymbolProvider, NtCurrentProcessId(), NtCurrentProcess());
            }

            PvAddTreeViewSections();

            if (PhGetIntegerPairSetting(L"MainWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"MainWindowPosition", L"MainWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, NULL);
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;
            PPV_WINDOW_SECTION section;

            PhSaveWindowPlacementToSetting(L"MainWindowPosition", L"MainWindowSize", hwndDlg);
            PvSaveWindowState(hwndDlg);

            if (PhGetIntegerSetting(L"MainWindowPageRestoreEnabled"))
                PhSetStringSetting2(L"MainWindowPage", &PvTabCurrentSection->Name);

            PhDeleteLayoutManager(&PvTabWindowLayoutManager);

            for (i = 0; i < PvTabSectionList->Count; i++)
            {
                section = PvTabSectionList->Items[i];
                PvDestroyTabSection(section);
            }

            PhDereferenceObject(PvTabSectionList);
            PvTabSectionList = NULL;

            PostQuitMessage(0);
        }
        break;
    case WM_DPICHANGED:
        {
            PvSetTreeViewImageList(hwndDlg, PvTabTreeControl);
        }
        break;
    case WM_SIZE:
        {
            PvTabWindowOnSize();
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_OPTIONS:
                {
                    PvShowOptionsWindow(hwndDlg);
                }
                break;
            case IDC_SECURITY:
                {
                    PhEditSecurity(
                        hwndDlg,
                        PhGetString(PvFileName),
                        L"FileObject",
                        PhpOpenFileSecurity,
                        PhpCloseFileSecurity,
                        NULL
                        );
                }
                break;
            }
        }
        break;
    case WM_DRAWITEM:
        {
            PDRAWITEMSTRUCT drawInfo = (PDRAWITEMSTRUCT)lParam;

            //if (drawInfo->CtlID == IDC_SEPARATOR)
            //{
            //    RECT rect;
            //
            //    rect = drawInfo->rcItem;
            //    rect.right = 2;
            //
            //    if (PhEnableThemeSupport)
            //    {
            //        switch (PhCsGraphColorMode)
            //        {
            //        case 0: // New colors
            //            {
            //                FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
            //                rect.left += 1;
            //                FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DSHADOW));
            //            }
            //            break;
            //        case 1: // Old colors
            //            {
            //                SetDCBrushColor(drawInfo->hDC, RGB(0, 0, 0));
            //                FillRect(drawInfo->hDC, &rect, PhGetStockBrush(DC_BRUSH));
            //            }
            //            break;
            //        }
            //    }
            //    else
            //    {
            //        FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
            //        rect.left += 1;
            //        FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DSHADOW));
            //    }
            //
            //    return TRUE;
            //}
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case TVN_KEYDOWN:
                {
                    LPNMTVKEYDOWN keydown = (LPNMTVKEYDOWN)lParam;

                    if (keydown->wVKey == 'K' && GetKeyState(VK_CONTROL) < 0)
                    {
                        PPV_WINDOW_SECTION section;

                        if (section = PvGetSelectedTabSection(NULL))
                            SendMessage(section->DialogHandle, WM_KEYDOWN, keydown->wVKey, 0);
                    }
                }
                break;
            case TVN_SELCHANGED:
                {
                    LPNMTREEVIEW treeview = (LPNMTREEVIEW)lParam;
                    PPV_WINDOW_SECTION section;

                    if (section = PvGetSelectedTabSection(treeview->itemNew.hItem))
                    {
                        PvEnterTabSectionView(section);
                    }
                }
                break;
            case NM_SETCURSOR:
                {
                    if (header->hwndFrom == PvTabTreeControl)
                    {
                        PhSetCursor(PhLoadArrowCursor());

                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }
                break;
            }
        }
        break;
    case WM_SETTINGCHANGE:
        if (HANDLE_COLORSCHEMECHANGE_MESSAGE(wParam, lParam, L"EnableThemeSupport", L"EnableThemeUseWindowsTheme"))
        {
            PhCreateThread2(PvReInitializeThemeThread, NULL);
        }
        break;
    }

    return FALSE;
}

NTSTATUS NTAPI PvReInitializeThemeThread(
    _In_ PVOID Context
    )
{
    BOOLEAN currentTheme;

    //currentTheme = PhShouldAppsUseDarkMode();
    currentTheme = PhGetAppsUseLightTheme();

    dprintf("PvReInitializeThemeThread: currentTheme = %d, PhEnableThemeSupport = %d\r\n", currentTheme, PhEnableThemeSupport);

    if (PhEnableThemeSupport != currentTheme)
    {
        PhEnableThemeSupport = currentTheme;

        PhEnableThemeAcrylicWindowSupport = PhEnableThemeAcrylicWindowSupport && PhEnableThemeSupport && PhIsThemeTransparencyEnabled();

        PhReInitializeTheme(PhEnableThemeSupport);
    };

    return STATUS_SUCCESS;
}

VOID PvTabWindowOnSize(
    VOID
    )
{
    PhLayoutManagerLayout(&PvTabWindowLayoutManager);

    if (PvTabSectionList && PvTabSectionList->Count != 0)
    {
        PvLayoutTabSectionView();
    }
}

HTREEITEM PvTreeViewInsertItem(
    _In_opt_ HTREEITEM HandleInsertAfter,
    _In_ PWSTR Text,
    _In_ PVOID Context
    )
{
    TV_INSERTSTRUCT insert;

    memset(&insert, 0, sizeof(TV_INSERTSTRUCT));
    insert.hParent = TVI_ROOT;
    insert.hInsertAfter = HandleInsertAfter;
    insert.item.mask = TVIF_TEXT | TVIF_PARAM;
    insert.item.pszText = Text;
    insert.item.lParam = (LPARAM)Context;

    return TreeView_InsertItem(PvTabTreeControl, &insert);
}

PPV_WINDOW_SECTION PvGetSelectedTabSection(
    _In_opt_ PVOID TreeItemHandle
    )
{
    TVITEM item;
    HTREEITEM itemHandle;

    if (TreeItemHandle)
        itemHandle = TreeItemHandle;
    else
        itemHandle = TreeView_GetSelection(PvTabTreeControl);

    memset(&item, 0, sizeof(TVITEM));
    item.mask = TVIF_PARAM | TVIF_HANDLE;
    item.hItem = itemHandle;

    if (!TreeView_GetItem(PvTabTreeControl, &item))
        return NULL;

    return (PPV_WINDOW_SECTION)item.lParam;
}

PPV_WINDOW_SECTION PvCreateTabSection(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    )
{
    PPV_WINDOW_SECTION section;

    section = PhAllocateZero(sizeof(PV_WINDOW_SECTION));
    PhInitializeStringRefLongHint(&section->Name, Name);
    section->Instance = Instance;
    section->Template = Template;
    section->DialogProc = DialogProc;
    section->Parameter = Parameter;
    section->TreeItemHandle = PvTreeViewInsertItem(TVI_LAST, Name, section);

    PhAddItemList(PvTabSectionList, section);

    return section;
}

PPV_WINDOW_SECTION PhOptionsCreateSectionAdvanced(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    )
{
    PPV_WINDOW_SECTION section;

    section = PhAllocateZero(sizeof(PV_WINDOW_SECTION));
    PhInitializeStringRefLongHint(&section->Name, Name);
    section->Instance = Instance;
    section->Template = Template;
    section->DialogProc = DialogProc;
    section->Parameter = Parameter;

    PhAddItemList(PvTabSectionList, section);

    return section;
}

VOID PvDestroyTabSection(
    _In_ PPV_WINDOW_SECTION Section
    )
{
    PhFree(Section);
}

PPV_WINDOW_SECTION PvFindTabSectionByName(
    _In_ PPH_STRINGREF Name
    )
{
    ULONG i;
    PPV_WINDOW_SECTION section;

    for (i = 0; i < PvTabSectionList->Count; i++)
    {
        section = PvTabSectionList->Items[i];

        if (PhEqualStringRef(&section->Name, Name, TRUE))
            return section;
    }

    return NULL;
}

VOID PvLayoutTabSectionView(
    VOID
    )
{
    if (PvTabCurrentSection && PvTabCurrentSection->DialogHandle)
    {
        RECT clientRect;

        GetClientRect(PvTabContainerControl, &clientRect);

        SetWindowPos(
            PvTabCurrentSection->DialogHandle,
            NULL,
            0,
            0,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
}

VOID PvEnterTabSectionView(
    _In_ PPV_WINDOW_SECTION NewSection
    )
{
    ULONG i;
    PPV_WINDOW_SECTION section;
    PPV_WINDOW_SECTION oldSection;
    HDWP containerDeferHandle;

    if (PvTabCurrentSection == NewSection)
        return;

    oldSection = PvTabCurrentSection;
    PvTabCurrentSection = NewSection;

    containerDeferHandle = BeginDeferWindowPos(PvTabSectionList->Count);

    PvEnterTabSectionViewInner(NewSection, &containerDeferHandle);
    PvLayoutTabSectionView();

    for (i = 0; i < PvTabSectionList->Count; i++)
    {
        section = PvTabSectionList->Items[i];

        if (section != NewSection)
            PvEnterTabSectionViewInner(section, &containerDeferHandle);
    }

    EndDeferWindowPos(containerDeferHandle);

    if (NewSection->DialogHandle)
        RedrawWindow(NewSection->DialogHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

VOID PvEnterTabSectionViewInner(
    _In_ PPV_WINDOW_SECTION Section,
    _Inout_ HDWP *ContainerDeferHandle
    )
{
    if (Section == PvTabCurrentSection && !Section->DialogHandle)
        PvCreateTabSectionDialog(Section);

    if (Section->DialogHandle)
    {
        if (Section == PvTabCurrentSection)
            *ContainerDeferHandle = DeferWindowPos(*ContainerDeferHandle, Section->DialogHandle, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW_ONLY | SWP_NOREDRAW);
        else
            *ContainerDeferHandle = DeferWindowPos(*ContainerDeferHandle, Section->DialogHandle, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW_ONLY | SWP_NOREDRAW);
    }
}

VOID PvCreateTabSectionDialog(
    _In_ PPV_WINDOW_SECTION Section
    )
{
    Section->DialogHandle = PhCreateDialogFromTemplate(
        PvTabContainerControl,
        DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD | WS_VISIBLE,
        Section->Instance,
        Section->Template,
        Section->DialogProc,
        Section->Parameter
        );

    PhInitializeWindowTheme(Section->DialogHandle);
}

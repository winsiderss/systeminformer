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
static HIMAGELIST PvTabTreeImageList = NULL;
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
    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &entry)) && entry->VirtualAddress)
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
        PvCreateTabSection(
            L"Exports",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEEXPORTS),
            PvPeExportsDlgProc,
            NULL
            );
    }

    // Resources page
    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_RESOURCE, &entry)) && entry->VirtualAddress)
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
    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_TLS, &entry)) && entry->VirtualAddress)
    {
        PvCreateTabSection(
            L"TLS",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_TLS),
            PvpPeTlsDlgProc,
            NULL
            );
    }

    // RICH header page
    // .NET executables don't include a RICH header.
    if (!(PvImageCor20Header && (PvImageCor20Header->Flags & COMIMAGE_FLAGS_NATIVE_ENTRYPOINT) == 0))
    {
        PvCreateTabSection(
            L"ProdID",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEPRODID),
            PvpPeProdIdDlgProc,
            NULL
            );
    }

    // Exceptions page
    {
        BOOLEAN has_exceptions = FALSE;

        if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            if (NT_SUCCESS(PhGetMappedImageLoadConfig32(&PvMappedImage, &config32)) &&
                RTL_CONTAINS_FIELD(config32, config32->Size, SEHandlerCount))
            {
                if (config32->SEHandlerCount && config32->SEHandlerTable)
                    has_exceptions = TRUE;
            }
        }
        else
        {
            if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_EXCEPTION, &entry)) && entry->VirtualAddress)
            {
                has_exceptions = TRUE;
            }
        }

        if (has_exceptions)
        {
            PvCreateTabSection(
                L"Exceptions",
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEEXCEPTIONS),
                PvpPeExceptionDlgProc,
                NULL
                );
        }
    }

    // Relocations page
    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_BASERELOC, &entry)) && entry->VirtualAddress)
    {
        PvCreateTabSection(
            L"Relocations",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PERELOCATIONS),
            PvpPeRelocationDlgProc,
            NULL
            );
    }

    // Certificates page
    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_SECURITY, &entry)) && entry->VirtualAddress)
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
    if (NT_SUCCESS(PhGetMappedImageDataEntry(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_DEBUG, &entry)) && entry->VirtualAddress)
    {
        PvCreateTabSection(
            L"Debug",
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEDEBUG),
            PvpPeDebugDlgProc,
            NULL
            );
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

VOID PvpSetImagelist(
    _In_ HWND tabWindow
    )
{
    HIMAGELIST hImageList;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(tabWindow);

    hImageList = PhImageListCreate (
        2,
        PhGetDpi(24, dpiValue),
        ILC_MASK | ILC_COLOR,
        1,
        1
        );

    if(PvTabTreeImageList)
        PhImageListDestroy (PvTabTreeImageList);

    if (hImageList)
    {
        PvTabTreeImageList = hImageList;
        TreeView_SetImageList (PvTabTreeControl, hImageList, TVSIL_NORMAL);
    }
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

            PvpSetImagelist(PvTabTreeControl);

            PhInitializeLayoutManager(&PvTabWindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&PvTabWindowLayoutManager, PvTabTreeControl, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            //PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDC_SEPARATOR), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&PvTabWindowLayoutManager, PvTabContainerControl, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDC_OPTIONS), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDC_SECURITY), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (PeEnableThemeSupport)
                PhInitializeWindowTheme(hwndDlg, TRUE);

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
                            (ULONG64)PvMappedImage.NtHeaders32->OptionalHeader.ImageBase,
                            PvMappedImage.NtHeaders32->OptionalHeader.SizeOfImage
                            );
                    }
                    else
                    {
                        PhLoadModuleSymbolProvider(
                            PvSymbolProvider,
                            fileName,
                            (ULONG64)PvMappedImage.NtHeaders->OptionalHeader.ImageBase,
                            PvMappedImage.NtHeaders->OptionalHeader.SizeOfImage
                            );
                    }

                    PhDereferenceObject(fileName);
                }

                PhLoadModulesForProcessSymbolProvider(PvSymbolProvider, NtCurrentProcessId());
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

            if (PvTabTreeImageList)
                PhImageListDestroy(PvTabTreeImageList);

            PostQuitMessage(0);
        }
        break;
    case WM_DPICHANGED:
        {
            PvpSetImagelist(PvTabTreeControl);
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
                        NULL,
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
            //                FillRect(drawInfo->hDC, &rect, GetStockBrush(DC_BRUSH));
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
            //case NM_SETCURSOR:
            //    {
            //        if (header->hwndFrom == OptionsTreeControl)
            //        {
            //            HCURSOR cursor = (HCURSOR)LoadImage(
            //                NULL,
            //                IDC_ARROW,
            //                IMAGE_CURSOR,
            //                0,
            //                0,
            //                LR_SHARED
            //                );
            //            if (cursor != GetCursor())
            //            {
            //                SetCursor(cursor);
            //            }
            //            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
            //            return TRUE;
            //        }
            //    }
            //    break;
            }
        }
        break;
    }

    return FALSE;
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

    PhInitializeWindowTheme(Section->DialogHandle, PeEnableThemeSupport);
}

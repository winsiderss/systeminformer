/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2017-2019 dmex
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

PPH_STRING PvpQueryModuleOrdinalName(
    _In_ PPH_STRING FileName,
    _In_ USHORT Ordinal
    )
{
    PPH_STRING exportName = NULL;
    PH_MAPPED_IMAGE mappedImage;

    if (NT_SUCCESS(PhLoadMappedImage(FileName->Buffer, NULL, TRUE, &mappedImage)))
    {
        PH_MAPPED_IMAGE_EXPORTS exports;
        PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
        PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;
        ULONG i;

        if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
        {
            for (i = 0; i < exports.NumberOfEntries; i++)
            {
                if (
                    NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)) &&
                    NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction))
                    )
                {
                    if (exportEntry.Ordinal == Ordinal)
                    {
                        if (exportEntry.Name)
                        {
                            exportName = PhZeroExtendToUtf16(exportEntry.Name);

                            if (exportName->Buffer[0] == '?')
                            {
                                PPH_STRING undecoratedName;

                                if (undecoratedName = PhUndecorateSymbolName(PvSymbolProvider, exportName->Buffer))
                                    PhMoveReference(&exportName, undecoratedName);
                            }
                        }
                        else
                        {
                            if (exportFunction.Function)
                            {
                                PPH_STRING symbolName;

                                // Try find the export name using symbols.
                                if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                                {
                                    symbolName = PhGetSymbolFromAddress(
                                        PvSymbolProvider,
                                        (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, exportFunction.Function),
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL
                                        );
                                }
                                else
                                {
                                    symbolName = PhGetSymbolFromAddress(
                                        PvSymbolProvider,
                                        (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, exportFunction.Function),
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL
                                        );
                                }

                                if (symbolName)
                                {
                                    static PH_STRINGREF unnamedText = PH_STRINGREF_INIT(L" (unnamed)");
                                    PH_STRINGREF exportNameText;
                                    PH_STRINGREF firstPart;
                                    PH_STRINGREF secondPart;

                                    if (PhSplitStringRefAtLastChar(&symbolName->sr, L'!', &firstPart, &secondPart))
                                        exportNameText = secondPart;
                                    else
                                        exportNameText = symbolName->sr;

                                    exportName = PhCreateString2(&exportNameText);
                                }
                            }
                        }

                        break;
                    }
                }
            }
        }

        PhUnloadMappedImage(&mappedImage);
    }

    return exportName;
}

VOID PvpProcessImports(
    _In_ HWND ListViewHandle,
    _In_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ BOOLEAN DelayImports,
    _Inout_ ULONG *Count
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

                    if (DelayImports)
                        name = PhFormatString(L"%S (Delay)", importDll.Name);
                    else
                        name = PhZeroExtendToUtf16(importDll.Name);

                    PhPrintUInt64(number, ++(*Count)); // HACK
                    lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, number, NULL);

                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, name->Buffer);
                    PhDereferenceObject(name);

                    if (importEntry.Name)
                    {
                        PPH_STRING importName;

                        importName = PhZeroExtendToUtf16(importEntry.Name);

                        if (importName->Buffer[0] == '?')
                        {
                            PPH_STRING undecoratedName;

                            if (undecoratedName = PhUndecorateSymbolName(PvSymbolProvider, importName->Buffer))
                                PhMoveReference(&importName, undecoratedName);
                        }

                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, importName->Buffer);
                        PhDereferenceObject(importName);

                        PhPrintUInt32(number, importEntry.NameHint);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, number);
                    }
                    else
                    {
                        PLDR_DATA_TABLE_ENTRY moduleLdrEntry = NULL;
                        PVOID moduleExportAddress = NULL;
                        PVOID importModuleDllBase = NULL;
                        PPH_STRING exportDllName = NULL;
                        PPH_STRING exportOrdinalName = NULL;
                        PPH_STRING exportSymbolName = NULL;

                        //PPH_STRING baseDirectory;
                        //
                        //if (baseDirectory = PhGetBaseDirectory(PvFileName))
                        //{
                        //    static DLL_DIRECTORY_COOKIE (WINAPI *AddDllDirectory_I)(
                        //        _In_ PCWSTR NewDirectory
                        //        );
                        //
                        //    if (AddDllDirectory_I = PhGetDllProcedureAddress(L"kernel32.dll", "AddDllDirectory", 0))
                        //    {
                        //        AddDllDirectory_I(baseDirectory->Buffer);
                        //    }
                        //}
                        //
                        //if (importModuleDllBase = LoadLibraryA(importDll.Name))
                        //{
                        //    moduleLdrEntry = PhFindLoaderEntry(importModuleDllBase, NULL, NULL);
                        //    moduleExportAddress = PhGetDllBaseProcedureAddress(importModuleDllBase, NULL, importEntry.Ordinal);
                        //    exportOrdinalName = PhGetExportNameFromOrdinal(importModuleDllBase, importEntry.Ordinal);
                        //}

                        if (exportDllName = PhConvertUtf8ToUtf16(importDll.Name))
                        {
                            PPH_STRING filePath;

                            if (filePath = PhSearchFilePath(exportDllName->Buffer, L".dll"))
                            {
                                PhMoveReference(&exportDllName, filePath);
                            }

                            exportOrdinalName = PvpQueryModuleOrdinalName(exportDllName, importEntry.Ordinal);
                        }

                        if (exportOrdinalName)
                        {
                            name = PhaFormatString(L"%s (Ordinal %u)", PhGetStringOrEmpty(exportOrdinalName), importEntry.Ordinal);
                            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(name));
                        }
                        else
                        {
                            if (moduleLdrEntry && moduleExportAddress)
                            {
                                if (PhLoadModuleSymbolProvider(
                                    PvSymbolProvider,
                                    moduleLdrEntry->FullDllName.Buffer,
                                    (ULONG64)importModuleDllBase,
                                    moduleLdrEntry->SizeOfImage
                                    ))
                                {
                                    exportSymbolName = PhGetSymbolFromAddress(
                                        PvSymbolProvider,
                                        (ULONG64)moduleExportAddress,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL
                                        );
                                }
                            }

                            if (exportSymbolName)
                            {
                                PH_STRINGREF firstPart;
                                PH_STRINGREF secondPart;

                                if (PhSplitStringRefAtLastChar(&exportSymbolName->sr, L'!', &firstPart, &secondPart))
                                    name = PhaFormatString(L"%s (Ordinal %u)", secondPart.Buffer, importEntry.Ordinal);
                                else
                                    name = PhaFormatString(L"%s (Ordinal %u)", exportSymbolName->Buffer, importEntry.Ordinal);

                                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, name->Buffer);
                            }
                            else
                            {
                                name = PhaFormatString(L"(Ordinal %u)", importEntry.Ordinal);
                                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, name->Buffer);
                            }
                        }

                        if (exportSymbolName) PhDereferenceObject(exportSymbolName);
                        if (exportOrdinalName) PhDereferenceObject(exportOrdinalName);
                        if (exportDllName) PhDereferenceObject(exportDllName);
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
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG count = 0;
            ULONG fallbackColumns[] = { 0, 1, 2 };
            HWND lvHandle;
            PH_MAPPED_IMAGE_IMPORTS imports;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 130, L"DLL");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 210, L"Name");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 50, L"Hint");
            PhSetExtendedListView(lvHandle);
            ExtendedListView_AddFallbackColumns(lvHandle, 3, fallbackColumns);
            PhLoadListViewColumnsFromSetting(L"ImageImportsListViewColumns", lvHandle);

            if (NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)))
            {
                PvpProcessImports(lvHandle, &imports, FALSE, &count);
            }

            if (NT_SUCCESS(PhGetMappedImageDelayImports(&imports, &PvMappedImage)))
            {
                PvpProcessImports(lvHandle, &imports, TRUE, &count);
            }

            ExtendedListView_SortItems(lvHandle);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageImportsListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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

/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s    2023
 *
 */

#include <peview.h>

typedef struct _PV_PE_CHPE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_PE_CHPE_CONTEXT, *PPV_PE_CHPE_CONTEXT;

VOID PvpCHPAddValue(
    _In_ HWND lvHandle,
    _In_ PWCHAR Name,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PPH_STRING Symbol
    )
{
    INT lvItemIndex;
    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, Name, NULL);
    if (Value)
        PhSetListViewSubItem(lvHandle, lvItemIndex, 1, Value->Buffer);
    if (Symbol)
        PhSetListViewSubItem(lvHandle, lvItemIndex, 2, Symbol->Buffer);
}

PPH_STRING PvpCHPERvaToSymbol(
    _In_ ULONG Rva
    )
{
    if (Rva)
    {
        PVOID va = NULL;

        if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            va = PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, Rva);
        else if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            va = PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, Rva);

        if (va)
        {
            PPH_STRING symbol;
            PH_SYMBOL_RESOLVE_LEVEL level;

            symbol = PhGetSymbolFromAddress(
                PvSymbolProvider,
                (ULONG64)va,
                &level,
                NULL,
                NULL,
                NULL
                );
            if (symbol)
            {
                return symbol;
            }
        }
    }

    return PhFormatString(L"0x%lx", Rva);
}

INT_PTR CALLBACK PvpPeCHPEDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PE_CHPE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_PE_CHPE_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 220, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Value");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 360, L"Symbol");
            PhSetExtendedListView(context->ListViewHandle);
            //PhLoadListViewColumnsFromSetting(L"CHPEListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            {
                PIMAGE_LOAD_CONFIG_DIRECTORY32 config32 = NULL;
                PIMAGE_CHPE_METADATA_X86 chpe32;

                PhGetMappedImageLoadConfig32(&PvMappedImage, &config32);
                // guaranteed upstream
                assert(config32);
                assert(RTL_CONTAINS_FIELD(config32, config32->Size, CHPEMetadataPointer));
                assert(config32->CHPEMetadataPointer);

                chpe32 = PhMappedImageVaToVa(&PvMappedImage, config32->CHPEMetadataPointer, NULL);
                if (chpe32)
                {
                    PvpCHPAddValue(
                        lvHandle,
                        L"Version",
                        PhFormatString(L"%lu", chpe32->Version),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Code address range offset",
                        PhFormatString(L"0x%lx", chpe32->CHPECodeAddressRangeOffset),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Code address range count",
                        PhFormatString(L"%lu", chpe32->CHPECodeAddressRangeOffset),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Exception handler",
                        PhFormatString(L"0x%lx", chpe32->WowA64ExceptionHandlerFunctionPointer),
                        PvpCHPERvaToSymbol(chpe32->WowA64ExceptionHandlerFunctionPointer)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch call",
                        PhFormatString(L"0x%lx", chpe32->WowA64DispatchCallFunctionPointer),
                        PvpCHPERvaToSymbol(chpe32->WowA64DispatchCallFunctionPointer)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch indirect call",
                        PhFormatString(L"0x%lx", chpe32->WowA64DispatchIndirectCallFunctionPointer),
                        PvpCHPERvaToSymbol(chpe32->WowA64DispatchIndirectCallFunctionPointer)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch indirect call guard",
                        PhFormatString(L"0x%lx", chpe32->WowA64DispatchIndirectCallCfgFunctionPointer),
                        PvpCHPERvaToSymbol(chpe32->WowA64DispatchIndirectCallCfgFunctionPointer)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch return",
                        PhFormatString(L"0x%lx", chpe32->WowA64DispatchRetFunctionPointer),
                        PvpCHPERvaToSymbol(chpe32->WowA64DispatchRetFunctionPointer)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch return leaf",
                        PhFormatString(L"0x%lx", chpe32->WowA64DispatchRetLeafFunctionPointer),
                        PvpCHPERvaToSymbol(chpe32->WowA64DispatchRetLeafFunctionPointer)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch jump",
                        PhFormatString(L"0x%lx", chpe32->WowA64DispatchJumpFunctionPointer),
                        PvpCHPERvaToSymbol(chpe32->WowA64DispatchJumpFunctionPointer)
                        );
                    if (chpe32->Version >= 2)
                    {
                        PvpCHPAddValue(lvHandle, L"Compiler IAT", PhFormatString(L"0x%lx", chpe32->CompilerIATPointer), NULL);
                    }
                    if (chpe32->Version >= 3)
                    {
                        PvpCHPAddValue(
                            lvHandle,
                            L"RDTSC",
                            PhFormatString(L"0x%lx", chpe32->WowA64RdtscFunctionPointer),
                            PvpCHPERvaToSymbol(chpe32->WowA64RdtscFunctionPointer)
                            );
                    }

                    if (chpe32->CHPECodeAddressRangeOffset && chpe32->CHPECodeAddressRangeCount)
                    {
                        PIMAGE_CHPE_RANGE_ENTRY table;

                        table = PhMappedImageRvaToVa(&PvMappedImage, chpe32->CHPECodeAddressRangeOffset, NULL);
                        if (table)
                        {
                            for (ULONG i = 0; i < chpe32->CHPECodeAddressRangeCount; i++)
                            {
                                ULONG start = table[i].StartOffset & ~1ul;
                                ULONG end = start + table[i].Length;
                                PPH_STRING startSym = PvpCHPERvaToSymbol(start);
                                PPH_STRING endSym = PvpCHPERvaToSymbol(end);

                                PvpCHPAddValue(
                                    lvHandle,
                                    PhFormatString(L"Code Address Range %lu", i)->Buffer,
                                    PhFormatString(L"[0x%lx, 0x%lx]%ls", start, end, table[i].NativeCode ? L" (native)" : L""),
                                    PhFormatString(L"(%ls, %ls)", startSym->Buffer, endSym->Buffer)
                                    );

                                PhDereferenceObject(startSym);
                                PhDereferenceObject(endSym);
                            }
                        }
                    }
                }
            }
            else
            {
                PIMAGE_LOAD_CONFIG_DIRECTORY64 config64 = NULL;
                PIMAGE_ARM64EC_METADATA chpe64;

                PhGetMappedImageLoadConfig64(&PvMappedImage, &config64);
                // guaranteed upstream
                assert(config64);
                assert(RTL_CONTAINS_FIELD(config64, config64->Size, CHPEMetadataPointer));
                assert(config64->CHPEMetadataPointer);

                chpe64 = PhMappedImageVaToVa(&PvMappedImage, config64->CHPEMetadataPointer, NULL);
                if (chpe64)
                {
                    PvpCHPAddValue(
                        lvHandle,
                        L"Version",
                        PhFormatString(L"%lu", chpe64->Version),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Code map",
                        PhFormatString(L"0x%lx", chpe64->CodeMap),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Code map count",
                        PhFormatString(L"%lu", chpe64->CodeMapCount),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Code ranges to entry points",
                        PhFormatString(L"0x%lx", chpe64->CodeRangesToEntryPoints),
                        NULL);
                    PvpCHPAddValue(
                        lvHandle,
                        L"Code ranges to entry points count",
                        PhFormatString(L"%lu", chpe64->CodeRangesToEntryPointsCount),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Redirection metadata",
                        PhFormatString(L"0x%lx", chpe64->RedirectionMetadata),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Redirection metadata count",
                        PhFormatString(L"%lu", chpe64->RedirectionMetadataCount),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Auxiliary IAT",
                        PhFormatString(L"0x%lx", chpe64->AuxiliaryIAT),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Auxiliary IAT copy",
                        PhFormatString(L"0x%lx", chpe64->AuxiliaryIATCopy),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Extra RFE table",
                        PhFormatString(L"0x%lx", chpe64->ExtraRFETable),
                        NULL
                       );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Extra RFE table size",
                        PhFormatString(L"0x%lx", chpe64->ExtraRFETableSize),
                        NULL
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Alternate entry point",
                        PhFormatString(L"0x%lx", chpe64->AlternateEntryPoint),
                        PvpCHPERvaToSymbol(chpe64->AlternateEntryPoint)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Get x64 information",
                        PhFormatString(L"0x%lx", chpe64->GetX64InformationFunctionPointer),
                        PvpCHPERvaToSymbol(chpe64->GetX64InformationFunctionPointer)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Set x64 information",
                        PhFormatString(L"0x%lx", chpe64->SetX64InformationFunctionPointer),
                        PvpCHPERvaToSymbol(chpe64->SetX64InformationFunctionPointer)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch call no redirect",
                        PhFormatString(L"0x%lx", chpe64->tbd__os_arm64x_dispatch_call_no_redirect),
                        PvpCHPERvaToSymbol(chpe64->tbd__os_arm64x_dispatch_call_no_redirect)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch return",
                        PhFormatString(L"0x%lx", chpe64->tbd__os_arm64x_dispatch_ret),
                        PvpCHPERvaToSymbol(chpe64->tbd__os_arm64x_dispatch_ret)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch call",
                        PhFormatString(L"0x%lx", chpe64->tbd__os_arm64x_dispatch_call),
                        PvpCHPERvaToSymbol(chpe64->tbd__os_arm64x_dispatch_call)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch indirect call",
                        PhFormatString(L"0x%lx", chpe64->tbd__os_arm64x_dispatch_icall),
                        PvpCHPERvaToSymbol(chpe64->tbd__os_arm64x_dispatch_icall)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch indirect call guard",
                        PhFormatString(L"0x%lx", chpe64->tbd__os_arm64x_dispatch_icall_cfg),
                        PvpCHPERvaToSymbol(chpe64->tbd__os_arm64x_dispatch_icall_cfg)
                        );
                    PvpCHPAddValue(
                        lvHandle,
                        L"Dispatch function pointer",
                        PhFormatString(L"0x%lx", chpe64->__os_arm64x_dispatch_fptr),
                        PvpCHPERvaToSymbol(chpe64->__os_arm64x_dispatch_fptr)
                        );

                    if (chpe64->RedirectionMetadata && chpe64->RedirectionMetadataCount)
                    {
                        PIMAGE_ARM64EC_REDIRECTION_ENTRY table;

                        table = PhMappedImageRvaToVa(&PvMappedImage, chpe64->RedirectionMetadata, NULL);
                        if (table)
                        {
                            for (ULONG i = 0; i < chpe64->RedirectionMetadataCount; i++)
                            {
                                PPH_STRING source = PvpCHPERvaToSymbol(table[i].Source);
                                PPH_STRING dest = PvpCHPERvaToSymbol(table[i].Destination);

                                PvpCHPAddValue(
                                    lvHandle,
                                    PhFormatString(L"Redirection entry %lu", i)->Buffer,
                                    PhFormatString(L"0x%lx -> 0x%lx", table[i].Source, table[i].Destination),
                                    PhFormatString(L"%ls -> %ls", source->Buffer, dest->Buffer)
                                    );

                                PhDereferenceObject(source);
                                PhDereferenceObject(dest);
                            }
                        }
                    }

                    if (chpe64->CodeRangesToEntryPoints && chpe64->CodeRangesToEntryPointsCount)
                    {
                        PIMAGE_ARM64EC_CODE_RANGE_ENTRY_POINT table;

                        table = PhMappedImageRvaToVa(&PvMappedImage, chpe64->CodeRangesToEntryPoints, NULL);
                        if (table)
                        {
                            for (ULONG i = 0; i < chpe64->CodeRangesToEntryPointsCount; i++)
                            {
                                PPH_STRING entry = PvpCHPERvaToSymbol(table[i].EntryPoint);

                                PvpCHPAddValue(
                                    lvHandle,
                                    PhFormatString(L"Code Entry Range%lu", i)->Buffer,
                                    PhFormatString(
                                        L"[0x%lx, 0x%lx] 0x%lx",
                                        table[i].StartRva,
                                        table[i].EndRva,
                                        table[i].EntryPoint),
                                    entry
                                    );

                                PhDereferenceObject(entry);
                            }
                        }
                    }
                }
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"CHPEListViewColumns", context->ListViewHandle);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}

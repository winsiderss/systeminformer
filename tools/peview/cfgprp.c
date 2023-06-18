/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2023
 *
 */

#include <peview.h>

typedef struct _PV_PE_CFG_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_PE_CFG_CONTEXT, *PPV_PE_CFG_CONTEXT;

VOID PvPeAddListViewCfgFunctionEntry(
    _In_ HWND ListViewHandle,
    _In_ ULONG64 Count,
    _In_ ULONGLONG Index,
    _In_ CFG_ENTRY_TYPE Type,
    _In_ PPH_MAPPED_IMAGE_CFG CfgConfig
    )
{
    INT lvItemIndex;
    ULONG64 displacement;
    PPH_STRING symbol;
    PPH_STRING symbolName = NULL;
    PH_SYMBOL_RESOLVE_LEVEL symbolResolveLevel = PhsrlInvalid;
    IMAGE_CFG_ENTRY cfgFunctionEntry = { 0 };
    PH_STRING_BUILDER stringBuilder;
    WCHAR value[PH_INT64_STR_LEN_1];

    if (!NT_SUCCESS(PhGetMappedImageCfgEntry(CfgConfig, Index, Type, &cfgFunctionEntry)))
        return;

    PhPrintUInt64(value, Count);
    lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);

    PhPrintPointer(value, UlongToPtr(cfgFunctionEntry.Rva));
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);

    switch (Type)
    {
    case ControlFlowGuardFunction:
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"Function");
        break;
    case ControlFlowGuardTakenIatEntry:
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"IATEntry");
        break;
    case ControlFlowGuardLongJump:
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"LongJump");
        break;
    }

    // Resolve name based on public symbols
    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        symbol = PhGetSymbolFromAddress(
            PvSymbolProvider,
            (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, cfgFunctionEntry.Rva),
            &symbolResolveLevel,
            NULL,
            &symbolName,
            &displacement
            );
    }
    else
    {
        symbol = PhGetSymbolFromAddress(
            PvSymbolProvider,
            (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, cfgFunctionEntry.Rva),
            &symbolResolveLevel,
            NULL,
            &symbolName,
            &displacement
            );
    }

    switch (symbolResolveLevel)
    {
    case PhsrlFunction:
        {
            if (displacement)
            {
                PhSetListViewSubItem(
                    ListViewHandle,
                    lvItemIndex,
                    3,
                    PhaFormatString(L"%s+0x%llx", PhGetStringOrEmpty(symbolName), displacement)->Buffer
                    );
            }
            else
            {
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PhGetStringOrEmpty(symbolName));
            }
        }
        break;
    case PhsrlModule:
    case PhsrlAddress:
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PhGetStringOrEmpty(symbol));
        break;
    default:
    case PhsrlInvalid:
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, L"(unnamed)");
        break;
    }

    if (symbolName)
        PhDereferenceObject(symbolName);
    PhDereferenceObject(symbol);

    // Add additional flags
    PhInitializeStringBuilder(&stringBuilder, 16);

    if (cfgFunctionEntry.SuppressedCall)
        PhAppendStringBuilder2(&stringBuilder, L"SuppressedCall, ");
    if (cfgFunctionEntry.ExportSuppressed)
        PhAppendStringBuilder2(&stringBuilder, L"ExportSuppressed, ");
    if (cfgFunctionEntry.LangExcptHandler)
        PhAppendStringBuilder2(&stringBuilder, L"LangExcptHandler, ");
    if (cfgFunctionEntry.Xfg)
        PhAppendStringBuilder2(&stringBuilder, L"XFG, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, PH_AUTO_T(PH_STRING, PhFinalStringBuilderString(&stringBuilder))->Buffer);

    if (cfgFunctionEntry.XfgHash)
    {
        PH_STRINGREF xfgHashString;
#ifdef _WIN64
        _ui64tow(cfgFunctionEntry.XfgHash, value, 16);
#else
        _ultow(cfgFunctionEntry.XfgHash, value, 16);
#endif
        PhInitializeStringRefLongHint(&xfgHashString, value);
        PhUpperStringRef(&xfgHashString);
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 5, PhGetStringRefZ(&xfgHashString));
    }
}

VOID PvpPeCgfEnumGuardFunctions(
    _In_ HWND ListViewHandle
    )
{
    PH_MAPPED_IMAGE_CFG cfgConfig = { 0 };
    ULONG64 count = 0;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (NT_SUCCESS(PhGetMappedImageCfg(&cfgConfig, &PvMappedImage)))
    {
        for (ULONGLONG i = 0; i < cfgConfig.NumberOfGuardFunctionEntries; i++)
        {
            PvPeAddListViewCfgFunctionEntry(ListViewHandle, ++count, i, ControlFlowGuardFunction, &cfgConfig);
        }

        for (ULONGLONG i = 0; i < cfgConfig.NumberOfGuardAdressIatEntries; i++)
        {
            PvPeAddListViewCfgFunctionEntry(ListViewHandle, ++count, i, ControlFlowGuardTakenIatEntry, &cfgConfig);
        }

        for (ULONGLONG i = 0; i < cfgConfig.NumberOfGuardLongJumpEntries; i++)
        {
            PvPeAddListViewCfgFunctionEntry(ListViewHandle, ++count, i, ControlFlowGuardLongJump, &cfgConfig);
        }
    }

    ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeCgfDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PE_CFG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_PE_CFG_CONTEXT));
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
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");

            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_RIGHT, 80, L"RVA");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 250, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Flags");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 100, L"XFG Hash");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageCfgListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvpPeCgfEnumGuardFunctions(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageCfgListViewColumns", context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

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

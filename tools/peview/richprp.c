/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2022
 *
 */

#include <peview.h>

typedef struct _PVP_PE_PRODUCTION_ID_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_PRODUCTION_ID_CONTEXT, *PPVP_PE_PRODUCTION_ID_CONTEXT;

PWSTR PvpGetProductIdName(
    _In_ ULONG ProductId
    )
{
    if (ProductId >= prodidAliasObj1400)
        return L"Visual Studio 2015 (14.0)";
    else if (ProductId >= prodidAliasObj1210)
        return L"Visual Studio 2013 (12.1)";
    else if (ProductId >= prodidAliasObj1200)
        return L"Visual Studio 2013 (12.0)";
    else if (ProductId >= prodidAliasObj1100)
        return L"Visual Studio 2012 (11.0)";
    else if (ProductId >= prodidAliasObj1010)
        return L"Visual Studio 2010 (10.1)";
    else if (ProductId >= prodidAliasObj1000)
        return L"Visual Studio 2010 (10.0)";
    else if (ProductId >= prodidLinker900) // prodidAliasObj900
        return L"Visual Studio 2008 (9.0)";
    else if (ProductId >= prodidLinker800) // prodidAliasObj710
        return L"Visual Studio 2003 (7.1)";
    else if (ProductId >= prodidLinker800) // prodidAliasObj710
        return L"Visual Studio 2003 (7.1)";
    else if (ProductId >= prodidAliasObj70)
        return L"Visual Studio 2002 (7.0)";
    else if (ProductId >= prodidAliasObj60)
        return L"Visual Studio 1998 (6.0)";
    else if (ProductId == 1)
        return L"Linker (Import Table)";

    return PhaFormatString(L"Report error (%lu)", ProductId)->Buffer;
}

PWSTR PvpGetProductIdComponent(
    _In_ ULONG ProductId
    )
{
    switch (ProductId)
    {
    case prodidUnknown: // linker generated unnamed ordinal export stubs with RVAs of zero? 
        return L"Linker generated export object";
    case prodidImport0:
        return L"Linker generated import object";
    case prodidResource:
        return L"Resource compiler object";

    case prodidAliasObj60:
        return L"ALIASOBJ (6.0)";
    case prodidAliasObj70:
        return L"ALIASOBJ (7.0)";
    case prodidAliasObj710:
    case prodidAliasObj710p:
        return L"ALIASOBJ (7.1)";
    case prodidAliasObj800:
        return L"ALIASOBJ (8.0)";
    case prodidAliasObj900:
        return L"ALIASOBJ (9.0)";
    case prodidAliasObj1000:
        return L"ALIASOBJ (10.0)";
    case prodidAliasObj1010:
        return L"ALIASOBJ (10.1)";
    case prodidAliasObj1100:
        return L"ALIASOBJ (11.0)";
    case prodidAliasObj1200:
        return L"ALIASOBJ (12.0)";
    case prodidAliasObj1210:
        return L"ALIASOBJ (12.1)";
    case prodidAliasObj1400:
        return L"ALIASOBJ (14.0)";

    case prodidCvtpgd1400:
        return L"Profile Guided Optimization (PGO) (14.0)";
    case prodidCvtpgd1500:
        return L"Profile Guided Optimization (PGO) (15.0)";
    case prodidCvtpgd1600:
        return L"Profile Guided Optimization (PGO) (16.0)";
    case prodidCvtpgd1610:
        return L"Profile Guided Optimization (PGO) (16.1)";
    case prodidCvtpgd1700:
        return L"Profile Guided Optimization (PGO) (17.0)";
    case prodidCvtpgd1800:
        return L"Profile Guided Optimization (PGO) (18.0)";
    case prodidCvtpgd1810:
        return L"Profile Guided Optimization (PGO) (18.1)";
    case prodidCvtpgd1900:
        return L"Profile Guided Optimization (PGO) (19.0)";

    case prodidCvtres800:
        return L"Resource File To COFF Object (8.0)";
    case prodidCvtres900:
        return L"Resource File To COFF Object (9.0)";
    case prodidCvtres1000:
        return L"Resource File To COFF Object (10.0)";
    case prodidCvtres1010:
        return L"Resource File To COFF Object (10.1)";
    case prodidCvtres1100:
        return L"Resource File To COFF Object (11.0)";
    case prodidCvtres1200:
        return L"Resource File To COFF Object (12.0)";
    case prodidCvtres1210:
        return L"Resource File To COFF Object (12.1)";
    case prodidCvtres1400:
        return L"Resource File To COFF Object (14.0)";

    case prodidExport800:
        return L"Export (8.0)";
    case prodidExport900:
        return L"Export (9.0)";
    case prodidExport1000:
        return L"Export (10.0)";
    case prodidExport1010:
        return L"Export (10.1)";
    case prodidExport1100:
        return L"Export (11.0)";
    case prodidExport1200:
        return L"Export (12.0)";
    case prodidExport1210:
        return L"Export (12.1)";
    case prodidExport1400:
        return L"Export (14.0)";

    case prodidImplib800:
        return L"Import library tool (LIB) (8.0)";
    case prodidImplib900:
        return L"Import library tool (LIB) (9.0)";
    case prodidImplib1000:
        return L"Import library tool (LIB) (10.0)";
    case prodidImplib1010:
        return L"Import library tool (LIB) (10.1)";
    case prodidImplib1100:
        return L"Import library tool (LIB) (11.0)";
    case prodidImplib1200:
        return L"Import library tool (LIB) (12.0)";
    case prodidImplib1210:
        return L"Import library tool (LIB) (12.1)";
    case prodidImplib1400:
        return L"Import library tool (LIB) (14.0)";

    case prodidLinker800:
        return L"Linker (8.0)";
    case prodidLinker900:
        return L"Linker (9.0)";
    case prodidLinker1000:
        return L"Linker (10.0)";
    case prodidLinker1010:
        return L"Linker (10.1)";
    case prodidLinker1100:
        return L"Linker (11.0)";
    case prodidLinker1200:
        return L"Linker (12.0)";
    case prodidLinker1210:
        return L"Linker (12.1)";
    case prodidLinker1400:
        return L"Linker (14.0)";

    case prodidMasm800:
        return L"MASM (8.0)";
    case prodidMasm900:
        return L"MASM (9.0)";
    case prodidMasm1000:
        return L"MASM (10.0)";
    case prodidMasm1010:
        return L"MASM (10.1)";
    case prodidMasm1100:
        return L"MASM (11.0)";
    case prodidMasm1200:
        return L"MASM (12.0)";
    case prodidMasm1210:
        return L"MASM (12.1)";
    case prodidMasm1400:
        return L"MASM (14.0)";

    case prodidUtc1400_C:
        return L"C files (14.0)";
    case prodidUtc1500_C:
        return L"C files (15.0)";
    case prodidUtc1610_C:
        return L"C files (16.1)";
    case prodidUtc1700_C:
        return L"C files (17.0)";
    case prodidUtc1800_C:
        return L"C files (18.0)";
    case prodidUtc1810_C:
        return L"C files (18.1)";
    case prodidUtc1900_C:
        return L"C files (19.0)";

    case prodidUtc1400_CPP:
        return L"CPP files (14.0)";
    case prodidUtc1500_CPP:
        return L"CPP files (15.0)";
    case prodidUtc1610_CPP:
        return L"CPP files (16.1)";
    case prodidUtc1700_CPP:
        return L"CPP files (17.0)";
    case prodidUtc1800_CPP:
        return L"CPP files (18.0)";
    case prodidUtc1810_CPP:
        return L"CPP files (18.1)";
    case prodidUtc1900_CPP:
        return L"CPP files (19.0)";

    case prodidUtc1500_CVTCIL_C:
        return L"CIL to Native Converter (C99) (15.0)";
    case prodidUtc1610_CVTCIL_C:
        return L"CIL to Native Converter (C99) (16.1)";
    case prodidUtc1700_CVTCIL_C:
        return L"CIL to Native Converter (C99) (17.0)";
    case prodidUtc1800_CVTCIL_C:
        return L"CIL to Native Converter (C11) (18.0)";
    case prodidUtc1810_CVTCIL_C:
        return L"CIL to Native Converter (C11) (18.1)";
    case prodidUtc1900_CVTCIL_C:
        return L"CIL to Native Converter (C11) (19.0)";

    case prodidUtc1500_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (15.0)";
    case prodidUtc1610_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (16.1)";
    case prodidUtc1700_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (17.0)";
    case prodidUtc1800_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (18.0)";
    case prodidUtc1810_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (18.1)";
    case prodidUtc1900_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (19.0)";

    case prodidUtc1500_LTCG_C:
        return L"Link-time Code Generation (C99) (15.0)";
    case prodidUtc1610_LTCG_C:
        return L"Link-time Code Generation (C99) (16.1)";
    case prodidUtc1700_LTCG_C:
        return L"Link-time Code Generation (C99) (17.0)";
    case prodidUtc1800_LTCG_C:
        return L"Link-time Code Generation (C11) (18.0)";
    case prodidUtc1810_LTCG_C:
        return L"Link-time Code Generation (C11) (18.1)";
    case prodidUtc1900_LTCG_C:
        return L"Link-time Code Generation (C11) (19.0)";

    case prodidUtc1500_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (15.0)";
    case prodidUtc1610_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (16.1)";
    case prodidUtc1700_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (17.0)";
    case prodidUtc1800_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (18.0)";
    case prodidUtc1810_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (18.1)";
    case prodidUtc1900_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (19.0)";

    case prodidUtc1500_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (15.0)";
    case prodidUtc1610_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (16.1)";
    case prodidUtc1700_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (17.0)";
    case prodidUtc1800_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (18.0)";
    case prodidUtc1810_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (18.1)";
    case prodidUtc1900_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (19.0)";

    case prodidUtc1500_POGO_I_C:
        return L"Profile Guided Optimization (Imput) (C11) (15.0)";
    case prodidUtc1610_POGO_I_C:
        return L"Profile Guided Optimization (Imput) (C11) (16.1)";
    case prodidUtc1700_POGO_I_C:
        return L"Profile Guided Optimization (Imput) (C11) (17.0)";
    case prodidUtc1800_POGO_I_C:
        return L"Profile Guided Optimization (Imput) (C11) (18.0)";
    case prodidUtc1810_POGO_I_C:
        return L"Profile Guided Optimization (Imput) (C11) (18.1)";
    case prodidUtc1900_POGO_I_C:
        return L"Profile Guided Optimization (Imput) (C11) (19.0)";

    case prodidUtc1400_POGO_O_C:
    case prodidUtc1500_POGO_O_C:
    case prodidUtc1610_POGO_O_C:
    case prodidUtc1700_POGO_O_C:
    case prodidUtc1800_POGO_O_C:
    case prodidUtc1810_POGO_O_C:
    case prodidUtc1900_POGO_O_C:
        return L"Profile Guided Optimization (Output) (C11)";

    case prodidUtc1400_POGO_I_CPP:
    case prodidUtc1500_POGO_I_CPP:
    case prodidUtc1610_POGO_I_CPP:
    case prodidUtc1700_POGO_I_CPP:
    case prodidUtc1800_POGO_I_CPP:
    case prodidUtc1810_POGO_I_CPP:
    case prodidUtc1900_POGO_I_CPP:
        return L"Profile Guided Optimization (Imput) (CPP)";

    case prodidUtc1400_POGO_O_CPP:
    case prodidUtc1500_POGO_O_CPP:
    case prodidUtc1610_POGO_O_CPP:
    case prodidUtc1700_POGO_O_CPP:
    case prodidUtc1800_POGO_O_CPP:
    case prodidUtc1810_POGO_O_CPP:
    case prodidUtc1900_POGO_O_CPP:
        return L"Profile Guided Optimization (Output) (C11)";
    }

    return PhaFormatString(L"Report error (%lu)", ProductId)->Buffer;
}

VOID PvpPeEnumProdEntries(
    _In_ HWND WindowHandle,
    _In_ HWND ListViewHandle
    )
{
    PH_MAPPED_IMAGE_PRODID prodids;
    PH_MAPPED_IMAGE_PRODID_ENTRY entry;
    ULONG count = 0;
    ULONG i;
    INT lvItemIndex;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (NT_SUCCESS(PhGetMappedImageProdIdHeader(&PvMappedImage, &prodids)))
    {
        PPH_STRING text;

        text = prodids.Valid ? prodids.Key : PhaConcatStrings2(PhGetStringOrEmpty(prodids.Key), L" (incorrect)");
        PhSetDialogItemText(WindowHandle, IDC_PRODCHECKSUM, PhGetStringOrEmpty(text));
        PhSetDialogItemText(WindowHandle, IDC_PRODHASH, PhGetStringOrEmpty(prodids.RawHash));
        PhSetDialogItemText(WindowHandle, IDC_PRODHASH2, PhGetStringOrEmpty(prodids.Hash));

        for (i = 0; i < prodids.NumberOfEntries; i++)
        {
            WCHAR number[PH_INT32_STR_LEN_1];

            entry = prodids.ProdIdEntries[i];

            if (!entry.ProductCount)
                continue;

            PhPrintUInt32(number, ++count);
            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, number, NULL);
            //PhSetListViewSubItem(lvHandle, lvItemIndex, 4, PvpGetProductIdName(entry.ProductId));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, PvpGetProductIdComponent(entry.ProductId));

            if (entry.ProductBuild)
            {
                PhPrintUInt32(number, entry.ProductBuild);
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, number);
            }

            PhPrintUInt32(number, entry.ProductCount);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, number);
        }

        PhFree(prodids.ProdIdEntries);
        PhClearReference(&prodids.Hash);
        PhClearReference(&prodids.Key);
    }

    //ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeProdIdDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_PRODUCTION_ID_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_PRODUCTION_ID_CONTEXT));
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
            HIMAGELIST listViewImageList;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Component");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Version");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Count");
            //PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Product");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageProdIdListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PRODID), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PRODCHECKSUM), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PRODHASH), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PRODHASH2), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            if (listViewImageList = PhImageListCreate(2, 20, ILC_MASK | ILC_COLOR, 1, 1))
                ListView_SetImageList(context->ListViewHandle, listViewImageList, LVSIL_SMALL);

            PvpPeEnumProdEntries(hwndDlg, context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageProdIdListViewColumns", context->ListViewHandle);
            PhDeleteLayoutManager(&context->LayoutManager);
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

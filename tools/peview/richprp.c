/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2020 dmex
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

PWSTR PvpGetProductIdName(
    _In_ ULONG ProductId
    )
{
    if (ProductId == 1)
        return L"Linker (Import Table)";
    else if (ProductId >= prodidAliasObj1400)
        return L"Visual Studio 2015 (14.00)";
    else if (ProductId >= prodidAliasObj1210)
        return L"Visual Studio 2013 (12.10)";
    else if (ProductId >= prodidAliasObj1200)
        return L"Visual Studio 2013 (12.00)";
    else if (ProductId >= prodidAliasObj1100)
        return L"Visual Studio 2012 (11.00)";
    else if (ProductId >= prodidAliasObj1010)
        return L"Visual Studio 2010 (10.10)";
    else if (ProductId >= prodidAliasObj1000)
        return L"Visual Studio 2010 (10.00)";
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

    return PhaFormatString(L"Report error (%lu)", ProductId)->Buffer;
}

PWSTR PvpGetProductIdComponent(
    _In_ ULONG ProductId
    )
{
    switch (ProductId)
    {
    case prodidImport0:
        return L"Linker generated import object";
    case prodidResource:
        return L"Resource compiler object";

    case prodidCvtpgd1400:
        return L"Profile Guided Optimization (PGO) (14.00)";
    case prodidCvtpgd1500:
        return L"Profile Guided Optimization (PGO) (15.00)";
    case prodidCvtpgd1600:
        return L"Profile Guided Optimization (PGO) (16.00)";
    case prodidCvtpgd1610:
        return L"Profile Guided Optimization (PGO) (16.10)";
    case prodidCvtpgd1700:
        return L"Profile Guided Optimization (PGO) (17.00)";
    case prodidCvtpgd1800:
        return L"Profile Guided Optimization (PGO) (18.00)";
    case prodidCvtpgd1810:
        return L"Profile Guided Optimization (PGO) (18.10)";
    case prodidCvtpgd1900:
        return L"Profile Guided Optimization (PGO) (19.00)";

    case prodidCvtres1010:
        return L"Resource File To COFF Object (10.10)";
    case prodidCvtres1100:
        return L"Resource File To COFF Object (11.00)";
    case prodidCvtres1200:
        return L"Resource File To COFF Object (12.00)";
    case prodidCvtres1210:
        return L"Resource File To COFF Object (12.10)";
    case prodidCvtres1400:
        return L"Resource File To COFF Object (14.00)";

    case prodidExport1010:
        return L"Export (10.10)";
    case prodidExport1100:
        return L"Export (11.00)";
    case prodidExport1200:
        return L"Export (12.00)";
    case prodidExport1210:
        return L"Export (12.10)";
    case prodidExport1400:
        return L"Export (14.00)";

    case prodidImplib900:
        return L"Import library tool (LIB) (9.0)";
    case prodidImplib1010:
        return L"Import library tool (LIB) (10.10)";
    case prodidImplib1100:
        return L"Import library tool (LIB) (11.00)";
    case prodidImplib1200:
        return L"Import library tool (LIB) (12.00)";
    case prodidImplib1210:
        return L"Import library tool (LIB) (12.10)";
    case prodidImplib1400:
        return L"Import library tool (LIB) (14.00)";

    case prodidLinker1010:
        return L"Linker (10.10)";
    case prodidLinker1100:
        return L"Linker (11.00)";
    case prodidLinker1200:
        return L"Linker (12.00)";
    case prodidLinker1210:
        return L"Linker (12.10)";
    case prodidLinker1400:
        return L"Linker (14.00)";

    case prodidMasm1010:
        return L"MASM (10.00)";
    case prodidMasm1100:
        return L"MASM (11.00)";
    case prodidMasm1200:
        return L"MASM (12.00)";
    case prodidMasm1210:
        return L"MASM (12.10)";
    case prodidMasm1400:
        return L"MASM (14.00)";

    case prodidUtc1610_C:
        return L"C files (16.10)";
    case prodidUtc1700_C:
        return L"C files (17.00)";
    case prodidUtc1800_C:
        return L"C files (18.00)";
    case prodidUtc1810_C:
        return L"C files (18.10)";
    case prodidUtc1900_C:
        return L"C files (19.00)";

    case prodidUtc1610_CPP:
        return L"CPP files (16.10)";
    case prodidUtc1700_CPP:
        return L"CPP files (17.00)";
    case prodidUtc1800_CPP:
        return L"CPP files (18.00)";
    case prodidUtc1810_CPP:
        return L"CPP files (18.10)";
    case prodidUtc1900_CPP:
        return L"CPP files (19.00)";

    case prodidUtc1610_CVTCIL_C:
        return L"CIL to Native Converter (C99) (16.10)";
    case prodidUtc1700_CVTCIL_C:
        return L"CIL to Native Converter (C99) (17.00)";
    case prodidUtc1800_CVTCIL_C:
        return L"CIL to Native Converter (C11) (18.00)";
    case prodidUtc1810_CVTCIL_C:
        return L"CIL to Native Converter (C11) (18.10)";
    case prodidUtc1900_CVTCIL_C:
        return L"CIL to Native Converter (C11) (19.00)";

    case prodidUtc1610_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (16.10)";
    case prodidUtc1700_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (17.00)";
    case prodidUtc1800_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (18.00)";
    case prodidUtc1810_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (18.10)";
    case prodidUtc1900_CVTCIL_CPP:
        return L"CIL to Native Converter (CPP) (19.00)";

    case prodidUtc1610_LTCG_C:
        return L"Link-time Code Generation (C11) (16.10)";
    case prodidUtc1700_LTCG_C:
        return L"Link-time Code Generation (C11) (17.00)";
    case prodidUtc1800_LTCG_C:
        return L"Link-time Code Generation (C11) (18.00)";
    case prodidUtc1810_LTCG_C:
        return L"Link-time Code Generation (C11) (18.10)";
    case prodidUtc1900_LTCG_C:
        return L"Link-time Code Generation (C11) (19.00)";

    case prodidUtc1610_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (16.10)";
    case prodidUtc1700_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (17.00)";
    case prodidUtc1800_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (18.00)";
    case prodidUtc1810_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (18.10)";
    case prodidUtc1900_LTCG_CPP:
        return L"Link-time Code Generation (CPP) (19.00)";

    case prodidUtc1610_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (16.10)";
    case prodidUtc1700_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (17.00)";
    case prodidUtc1800_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (18.00)";
    case prodidUtc1810_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (18.10)";
    case prodidUtc1900_LTCG_MSIL:
        return L"Link-time Code Generation (MSIL) (19.00)";

    case prodidUtc1610_POGO_I_C:
    case prodidUtc1700_POGO_I_C:
    case prodidUtc1800_POGO_I_C:
    case prodidUtc1810_POGO_I_C:
    case prodidUtc1900_POGO_I_C:
        return L"Profile Guided Optimization (Imput) (C11)";

    case prodidUtc1610_POGO_O_C:
    case prodidUtc1700_POGO_O_C:
    case prodidUtc1800_POGO_O_C:
    case prodidUtc1810_POGO_O_C:
    case prodidUtc1900_POGO_O_C:
        return L"Profile Guided Optimization (Output) (C11)";

    case prodidUtc1610_POGO_I_CPP:
    case prodidUtc1700_POGO_I_CPP:
    case prodidUtc1800_POGO_I_CPP:
    case prodidUtc1810_POGO_I_CPP:
    case prodidUtc1900_POGO_I_CPP:
        return L"Profile Guided Optimization (Imput) (C11)";

    case prodidUtc1610_POGO_O_CPP:
    case prodidUtc1700_POGO_O_CPP:
    case prodidUtc1800_POGO_O_CPP:
    case prodidUtc1810_POGO_O_CPP:
    case prodidUtc1900_POGO_O_CPP:
        return L"Profile Guided Optimization (Output) (C11)";
    }

    return PhaFormatString(L"Report error (%lu)", ProductId)->Buffer;
}

INT_PTR CALLBACK PvpPeProdIdDlgProc(
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
            PH_MAPPED_IMAGE_PRODID prodids;
            PH_MAPPED_IMAGE_PRODID_ENTRY entry;
            HWND lvHandle;
            ULONG count = 0;
            ULONG i;
            INT lvItemIndex;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Product");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Component");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Version");
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Count");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"ImageProdIdListViewColumns", lvHandle);

            if (NT_SUCCESS(PhGetMappedImageProdIdHeader(&PvMappedImage, &prodids)))
            {
                PhSetDialogItemText(hwndDlg, IDC_PRODCHECKSUM, PhGetStringOrEmpty(prodids.Key));
                PhSetDialogItemText(hwndDlg, IDC_PRODHASH, PhGetStringOrEmpty(prodids.Hash));

                for (i = 0; i < prodids.NumberOfEntries; i++)
                {
                    WCHAR number[PH_INT32_STR_LEN_1];

                    entry = prodids.ProdIdEntries[i];

                    if (!entry.ProductCount)
                        continue;

                    PhPrintUInt32(number, ++count);
                    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, number, NULL);
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 1, PvpGetProductIdName(entry.ProductId));
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PvpGetProductIdComponent(entry.ProductId));

                    PhPrintUInt32(number, entry.ProductBuild);
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 3, number);

                    PhPrintUInt32(number, entry.ProductCount);
                    PhSetListViewSubItem(lvHandle, lvItemIndex, 4, number);
                }

                PhFree(prodids.ProdIdEntries);
                PhClearReference(&prodids.Hash);
                PhClearReference(&prodids.Key);
            }

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageProdIdListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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

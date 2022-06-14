/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2019
 *
 */

#include <peview.h>

PWSTR PvpGetDynamicTagName(
    _In_ LONGLONG Tag
    )
{
    switch (Tag)
    {
    case DT_NULL:
        return L"NULL";
    case DT_NEEDED:
        return L"NEEDED";
    case DT_PLTRELSZ:
        return L"PLTRELSZ";
    case DT_PLTGOT:
        return L"PLTGOT";
    case DT_HASH:
        return L"HASH";
    case DT_STRTAB:
        return L"STRTAB";
    case DT_SYMTAB:
        return L"SYMTAB";
    case DT_RELA:
        return L"RELA";
    case DT_RELASZ:
        return L"RELASZ";
    case DT_RELAENT:
        return L"RELAENT";
    case DT_STRSZ:
        return L"STRSZ";
    case DT_SYMENT:
        return L"SYMENT";
    case DT_INIT:
        return L"INIT";
    case DT_FINI:
        return L"FINI";
    case DT_SONAME:
        return L"SONAME";
    case DT_RPATH:
        return L"RPATH";
    case DT_SYMBOLIC:
        return L"SYMBOLIC";
    case DT_REL:
        return L"REL";
    case DT_RELSZ:
        return L"RELSZ";
    case DT_RELENT:
        return L"RELENT";
    case DT_PLTREL:
        return L"PLTREL";
    case DT_DEBUG:
        return L"DEBUG";
    case DT_TEXTREL:
        return L"TEXTREL";
    case DT_JMPREL:
        return L"JMPREL";
    case DT_INIT_ARRAY:
        return L"INIT_ARRAY";
    case DT_FINI_ARRAY:
        return L"FINI_ARRAY";
    case DT_INIT_ARRAYSZ:
        return L"INIT_ARRAYSZ";
    case DT_FINI_ARRAYSZ:
        return L"FINI_ARRAYSZ";
    case DT_RUNPATH:
        return L"RUNPATH";
    case DT_FLAGS:
        return L"FLAGS";
    case DT_PREINIT_ARRAY:
        return L"PREINIT_ARRAY";
    case DT_PREINIT_ARRAYSZ:
        return L"PREINIT_ARRAYSZ";
    case OLD_DT_LOOS:
        return L"OLD_DT_LOOS";
    case DT_LOOS:
        return L"LOOS";
    case DT_HIOS:
        return L"HIOS";
    case DT_VALRNGLO:
        return L"VALRNGLO";
    case DT_VALRNGHI:
        return L"VALRNGHI";
    case DT_ADDRRNGLO:
        return L"ADDRRNGLO";
    case DT_ADDRRNGHI:
        return L"ADDRRNGHI";
    case DT_GNU_HASH:
        return L"GNU_HASH";
    case DT_VERSYM:
        return L"VERSYM";
    case DT_RELACOUNT:
        return L"RELACOUNT";
    case DT_RELCOUNT:
        return L"RELCOUNT";
    case DT_FLAGS_1:
        return L"FLAGS_1";
    case DT_VERDEF:
        return L"VERDEF";
    case DT_VERDEFNUM:
        return L"VERDEFNUM";
    case DT_VERNEED:
        return L"VERNEED";
    case DT_VERNEEDNUM:
        return L"VERNEEDNUM";
    case DT_LOPROC:
        return L"LOPROC";
    case DT_HIPROC:
        return L"HIPROC";
    }

    return L"***ERROR***";
}

VOID PvpProcessElfDynamic(
    _In_ HWND ListViewHandle
    )
{
    PPH_LIST dynamics;
    ULONG count = 0;

    PhGetMappedWslImageDynamic(&PvMappedImage, &dynamics);

    for (ULONG i = 0; i < dynamics->Count; i++)
    {
        PPH_ELF_IMAGE_DYNAMIC_ENTRY dynamic = dynamics->Items[i];
        INT lvItemIndex;
        WCHAR value[PH_PTR_STR_LEN_1];

        PhPrintUInt32(value, ++count);
        lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);

        PhPrintPointer(value, (PVOID)dynamic->Tag);
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);
        //PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, PhaFormatString(L"0x%016llx", (PVOID)dynamic->Tag)->Buffer);
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PvpGetDynamicTagName(dynamic->Tag));
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PhGetStringOrEmpty(dynamic->Value));
    }

    PhFreeMappedWslImageDynamic(dynamics);
}

INT_PTR CALLBACK PvpExlfDynamicDlgProc(
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

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_RIGHT, 80, L"Tag");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 150, L"Type");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 250, L"Value");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"DynamicWslListViewColumns", lvHandle);

            PvpProcessElfDynamic(lvHandle);
            ExtendedListView_SortItems(lvHandle);
            
            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"DynamicWslListViewColumns", GetDlgItem(hwndDlg, IDC_LIST));
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

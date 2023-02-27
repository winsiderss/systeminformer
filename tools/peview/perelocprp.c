/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2022
 *
 */

#include <peview.h>

typedef struct _PVP_PE_RELOCATION_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_RELOCATION_CONTEXT, *PPVP_PE_RELOCATION_CONTEXT;

VOID PvEnumerateRelocationEntries(
    _In_ HWND ListViewHandle
    )
{
    ULONG count = 0;
    PH_MAPPED_IMAGE_RELOC relocations;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (NT_SUCCESS(PhGetMappedImageRelocations(&PvMappedImage, &relocations)))
    {
        for (ULONG i = 0; i < relocations.NumberOfEntries; i++)
        {
            PPH_IMAGE_RELOC_ENTRY entry = PTR_ADD_OFFSET(relocations.RelocationEntries, i * sizeof(PH_IMAGE_RELOC_ENTRY));
            INT lvItemIndex;
            WCHAR value[PH_INT64_STR_LEN_1];
            PPH_STRING symbol;

            PhPrintUInt64(value, ++count);
            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);
            //PhPrintPointer(value, UlongToPtr(entry->BlockRva));
            //PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);
            //PhPrintPointer(value, UlongToPtr(entry->Offset));
            //PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, value);
            PhPrintPointer(value, PTR_ADD_OFFSET(entry->BlockRva, entry->Record.Offset));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);

            switch (entry->Record.Type)
            {
            case IMAGE_REL_BASED_ABSOLUTE:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"ABS");
                break;
            case IMAGE_REL_BASED_HIGH:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"HIGH");
                break;
            case IMAGE_REL_BASED_LOW:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"LOW");
                break;
            case IMAGE_REL_BASED_HIGHLOW:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"HIGHLOW");
                break;
            case IMAGE_REL_BASED_DIR64:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"DIR64");
                break;
            case IMAGE_REL_BASED_ARM_MOV32:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"MOV32");
                break;
            case IMAGE_REL_BASED_THUMB_MOV32:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"MOV32(T)");
                break;
            }

            if (entry->BlockRva)
            {
                PIMAGE_SECTION_HEADER directorySection;

                directorySection = PhMappedImageRvaToSection(
                    &PvMappedImage,
                    PtrToUlong(PTR_ADD_OFFSET(entry->BlockRva, entry->Record.Offset))
                    );

                if (directorySection)
                {
                    WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

                    if (PhGetMappedImageSectionName(
                        directorySection,
                        sectionName,
                        RTL_NUMBER_OF(sectionName),
                        NULL
                        ))
                    {
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, sectionName);
                    }
                }
            }

            symbol = PhGetSymbolFromAddress(
                PvSymbolProvider,
                (ULONG64)entry->ImageBaseVa,
                NULL,
                NULL,
                NULL,
                NULL
                );

            if (symbol)
            {
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, symbol->Buffer);
                PhDereferenceObject(symbol);
            }

            if (entry->MappedImageVa)
            {
                ULONG64 reloc = MAXULONG64;

                __try
                {
                    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                    {
                        reloc = *(PULONG64)entry->MappedImageVa;
                    }
                    else if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                    {
                        reloc = *(PULONG32)entry->MappedImageVa;
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    reloc = MAXULONG64;
                }

                if (reloc != MAXULONG64)
                {
                    symbol = PhGetSymbolFromAddress(
                        PvSymbolProvider,
                        reloc,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );

                    if (symbol)
                    {
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 5, symbol->Buffer);
                        PhDereferenceObject(symbol);
                    }
                }
            }
        }
    }

    //ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeRelocationDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_RELOCATION_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_RELOCATION_CONTEXT));
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
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 50, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"RVA");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Section");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 140, L"Symbol");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 140, L"RelocationSymbol");
            PhSetExtendedListView(context->ListViewHandle);
            //PhLoadListViewColumnsFromSetting(L"ImageRelocationsListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvEnumerateRelocationEntries(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
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
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageRelocationsListViewColumns", context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
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

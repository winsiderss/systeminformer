/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2021 dmex
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

typedef struct _PVP_PE_EXCEPTION_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_EXCEPTION_CONTEXT, *PPVP_PE_EXCEPTION_CONTEXT;

VOID PvEnumerateExceptionEntries(
    _In_ HWND WindowHandle,
    _In_ HWND ListViewHandle
    )
{
    ULONG count = 0;
    ULONG imageMachine;
    PH_MAPPED_IMAGE_EXCEPTIONS exceptions;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        imageMachine = PvMappedImage.NtHeaders32->FileHeader.Machine;
    else
        imageMachine = PvMappedImage.NtHeaders->FileHeader.Machine;

    if (!NT_SUCCESS(PhGetMappedImageExceptions(&PvMappedImage, &exceptions)))
    {
        return;
    }

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (imageMachine == IMAGE_FILE_MACHINE_I386)
    {
        for (ULONG i = 0; i < exceptions.NumberOfEntries; i++)
        {
            ULONG entry = *(PULONG)PTR_ADD_OFFSET(exceptions.ExceptionEntries, i * sizeof(ULONG));
            INT lvItemIndex;
            PPH_STRING symbol;
            PPH_STRING symbolName = NULL;
            WCHAR value[PH_INT64_STR_LEN_1];

            PhPrintUInt64(value, ++count);
            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);

            PhPrintPointer(value, UlongToPtr(entry));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);

            symbol = PhGetSymbolFromAddress(
                PvSymbolProvider,
                (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, entry),
                NULL,
                NULL,
                &symbolName,
                NULL
                );

            if (symbolName)
            {
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, symbolName->Buffer);
                PhDereferenceObject(symbolName);
            }

            if (symbol) PhDereferenceObject(symbol);

            if (entry)
            {
                PIMAGE_SECTION_HEADER directorySection;

                directorySection = PhMappedImageRvaToSection(
                    &PvMappedImage,
                    entry
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
        }
    }
    else if (imageMachine == IMAGE_FILE_MACHINE_AMD64)
    {
        for (ULONG i = 0; i < exceptions.NumberOfEntries; i++)
        {
            PIMAGE_RUNTIME_FUNCTION_ENTRY entry = PTR_ADD_OFFSET(exceptions.ExceptionDirectory, i * sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY));
            INT lvItemIndex;
            PPH_STRING symbol;
            PPH_STRING symbolName = NULL;
            WCHAR value[PH_INT64_STR_LEN_1];

            PhPrintUInt64(value, ++count);
            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);

            PhPrintPointer(value, UlongToPtr(entry->BeginAddress));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);
            PhPrintPointer(value, UlongToPtr(entry->EndAddress));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, value);
            PhPrintPointer(value, UlongToPtr(entry->UnwindData)); // UnwindInfoAddress
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, value);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, PhaFormatSize(entry->EndAddress - entry->BeginAddress, ULONG_MAX)->Buffer);

            symbol = PhGetSymbolFromAddress(
                PvSymbolProvider,
                (ULONG64)PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, entry->BeginAddress),
                NULL,
                NULL,
                &symbolName,
                NULL
                );

            if (symbolName)
            {
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 5, symbolName->Buffer);
                PhDereferenceObject(symbolName);
            }

            if (symbol) PhDereferenceObject(symbol);

            if (entry->BeginAddress)
            {
                PIMAGE_SECTION_HEADER directorySection;

                directorySection = PhMappedImageRvaToSection(
                    &PvMappedImage,
                    entry->BeginAddress
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
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 6, sectionName);
                    }
                }
            }
        }
    }

    //ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeExceptionDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_EXCEPTION_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_EXCEPTION_CONTEXT));
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
            ULONG imageMachine;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhSetExtendedListView(context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                imageMachine = PvMappedImage.NtHeaders32->FileHeader.Machine;
            else
                imageMachine = PvMappedImage.NtHeaders->FileHeader.Machine;

            if (imageMachine == IMAGE_FILE_MACHINE_I386)
            {
                PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"SEH Handler");
                PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Symbol");
                PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Section");
                PhLoadListViewColumnsFromSetting(L"ImageExceptions32ListViewColumns", context->ListViewHandle);
            }
            else if (imageMachine == IMAGE_FILE_MACHINE_AMD64)
            {
                PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"RVA (start)");
                PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"RVA (end)");
                PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 200, L"Data");
                PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Size");
                PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 200, L"Symbol");
                PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 100, L"Section");
                PhLoadListViewColumnsFromSetting(L"ImageExceptions64ListViewColumns", context->ListViewHandle);
            }

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvEnumerateExceptionEntries(hwndDlg, context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            ULONG imageMachine;

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                imageMachine = PvMappedImage.NtHeaders32->FileHeader.Machine;
            else
                imageMachine = PvMappedImage.NtHeaders->FileHeader.Machine;

            if (imageMachine == IMAGE_FILE_MACHINE_I386)
            {
                PhSaveListViewColumnsToSetting(L"ImageExceptions32ListViewColumns", context->ListViewHandle);
            }
            else if (imageMachine == IMAGE_FILE_MACHINE_AMD64)
            {
                PhSaveListViewColumnsToSetting(L"ImageExceptions64ListViewColumns", context->ListViewHandle);
            }

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
    }

    return FALSE;
}

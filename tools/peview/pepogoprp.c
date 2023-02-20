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

#include "..\thirdparty\ssdeep\fuzzy.h"
#include "..\thirdparty\tlsh\tlsh_wrapper.h"

typedef struct _PVP_PE_POGO_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_POGO_CONTEXT, *PPVP_PE_POGO_CONTEXT;

typedef struct _PVP_PE_CRT_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_CRT_CONTEXT, *PPVP_PE_CRT_CONTEXT;

VOID PvEnumerateImagePogoSections(
    _In_ HWND WindowHandle,
    _In_ HWND ListViewHandle
    )
{
    ULONG count = 0;
    PH_MAPPED_IMAGE_DEBUG_POGO pogo;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (NT_SUCCESS(PhGetMappedImagePogo(&PvMappedImage, &pogo)))
    {
        for (ULONG i = 0; i < pogo.NumberOfEntries; i++)
        {
            PPH_IMAGE_DEBUG_POGO_ENTRY entry = PTR_ADD_OFFSET(pogo.PogoEntries, i * sizeof(PH_IMAGE_DEBUG_POGO_ENTRY));
            PIMAGE_SECTION_HEADER section;
            PVOID imageSectionData;
            PPH_STRING hashString;
            INT lvItemIndex;
            WCHAR value[PH_INT64_STR_LEN_1];

            PhPrintUInt64(value, ++count);
            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, entry->Name);
            PhPrintPointer(value, UlongToPtr(entry->Rva));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, value);
            PhPrintPointer(value, PTR_ADD_OFFSET(entry->Rva, entry->Size));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, value);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, PhaFormatSize(entry->Size, ULONG_MAX)->Buffer);

            if (section = PhMappedImageRvaToSection(&PvMappedImage, entry->Rva))
            {
                WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

                if (PhGetMappedImageSectionName(section, sectionName, RTL_NUMBER_OF(sectionName), NULL))
                {
                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 5, sectionName);
                }
            }

            if (imageSectionData = PhMappedImageRvaToVa(&PvMappedImage, entry->Rva, NULL))
            {
                if (hashString = PvHashBuffer(imageSectionData, entry->Size))
                {
                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 6, PhGetString(hashString));
                    PhDereferenceObject(hashString);
                }
            }
            else
            {
                // TODO: POGO-PGU can have an RVA outside image sections. For example:
                // C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\VC\vcpackages\vcpkgsrv.exe
            }

            __try
            {
                PVOID imageSectionData;
                PPH_STRING entropyString;
                DOUBLE imageSectionEntropy;

                if (imageSectionData = PhMappedImageRvaToVa(&PvMappedImage, entry->Rva, NULL))
                {
                    if (PhCalculateEntropy(imageSectionData, entry->Size, &imageSectionEntropy, NULL))
                    {
                        entropyString = PhFormatEntropy(imageSectionEntropy, 2, 0, 0);
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 7, entropyString->Buffer);
                        PhDereferenceObject(entropyString);
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                PPH_STRING message;

                //message = PH_AUTO(PhGetNtMessage(GetExceptionCode()));
                message = PH_AUTO(PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode()))); // WIN32_FROM_NTSTATUS

                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 7, PhGetStringOrEmpty(message));
            }

            __try
            {
                PVOID imageSectionData;
                PPH_STRING ssdeepHashString = NULL;

                if (imageSectionData = PhMappedImageRvaToVa(&PvMappedImage, entry->Rva, NULL))
                {
                    fuzzy_hash_buffer(imageSectionData, entry->Size, &ssdeepHashString);

                    if (!PhIsNullOrEmptyString(ssdeepHashString))
                    {
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 8, ssdeepHashString->Buffer);
                        PhDereferenceObject(ssdeepHashString);
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                PPH_STRING message;

                //message = PH_AUTO(PhGetNtMessage(GetExceptionCode()));
                message = PH_AUTO(PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode()))); // WIN32_FROM_NTSTATUS

                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 8, PhGetStringOrEmpty(message));
            }

            __try
            {
                PVOID imageSectionData;
                PPH_STRING tlshHashString = NULL;

                if (imageSectionData = PhMappedImageRvaToVa(&PvMappedImage, entry->Rva, NULL))
                {
                    //
                    // This can fail in TLSH library during finalization when
                    // "buckets must be more than 50% non-zero" (see: tlsh_impl.cpp)
                    //
                    PvGetTlshBufferHash(imageSectionData, entry->Size, &tlshHashString);

                    if (!PhIsNullOrEmptyString(tlshHashString))
                    {
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 9, tlshHashString->Buffer);
                        PhDereferenceObject(tlshHashString);
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                PPH_STRING message;

                //message = PH_AUTO(PhGetNtMessage(GetExceptionCode()));
                message = PH_AUTO(PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode()))); // WIN32_FROM_NTSTATUS

                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 9, PhGetStringOrEmpty(message));
            }
        }

        PhFree(pogo.PogoEntries);
    }

    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeDebugPogoDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_POGO_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_POGO_CONTEXT));
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
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"RVA (start)");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"RVA (end)");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Size");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 100, L"Section");
            PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 80, L"Hash");
            PhAddListViewColumn(context->ListViewHandle, 7, 7, 7, LVCFMT_LEFT, 100, L"Entropy");
            PhAddListViewColumn(context->ListViewHandle, 8, 8, 8, LVCFMT_LEFT, 80, L"SSDEEP");
            PhAddListViewColumn(context->ListViewHandle, 9, 9, 9, LVCFMT_LEFT, 80, L"TLSH");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageDebugPogoListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvEnumerateImagePogoSections(hwndDlg, context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageDebugPogoListViewColumns", context->ListViewHandle);

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

_Success_(return)
BOOLEAN PvGetCRTFunctionSegment(
    _Out_ PULONG Count,
    _Out_ PULONG_PTR Address
    )
{
    PIMAGE_DEBUG_POGO_SIGNATURE debugEntry;
    ULONG debugEntryLength = 0;
    ULONG debugEntrySize = 0;
    ULONG_PTR first = 0;
    ULONG_PTR last = 0;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        debugEntrySize = sizeof(ULONG);
    else
        debugEntrySize = sizeof(ULONGLONG);

    if (NT_SUCCESS(PhGetMappedImageDebugEntryByType(
        &PvMappedImage,
        IMAGE_DEBUG_TYPE_POGO,
        &debugEntryLength,
        &debugEntry
        )))
    {
        PIMAGE_DEBUG_POGO_ENTRY debugPogoEntry;

        if (debugEntry->Signature != IMAGE_DEBUG_POGO_SIGNATURE_LTCG && debugEntry->Signature != IMAGE_DEBUG_POGO_SIGNATURE_PGU)
        {
            if (!(debugEntry->Signature == 0 && debugEntryLength > sizeof(IMAGE_DEBUG_POGO_SIGNATURE)))
                return FALSE;
        }

        debugPogoEntry = PTR_ADD_OFFSET(debugEntry, sizeof(IMAGE_DEBUG_POGO_SIGNATURE));

        while ((ULONG_PTR)debugPogoEntry < (ULONG_PTR)PTR_ADD_OFFSET(debugEntry, debugEntryLength))
        {
            if (!(debugPogoEntry->Rva && debugPogoEntry->Size))
                break;

            if (strncmp(debugPogoEntry->Name, ".CRT$", 5) == 0)
            {
                if (first == 0)
                    first = debugPogoEntry->Rva;
                if (first > debugPogoEntry->Rva)
                    first = debugPogoEntry->Rva;
                if (last == 0)
                    last = debugPogoEntry->Rva;
                if (last < debugPogoEntry->Rva)
                    last = debugPogoEntry->Rva;
                //length += debugPogoEntry->Size;
            }

            debugPogoEntry = PTR_ADD_OFFSET(debugPogoEntry, ALIGN_UP(UFIELD_OFFSET(IMAGE_DEBUG_POGO_ENTRY, Name) + strlen(debugPogoEntry->Name) + sizeof(ANSI_NULL), ULONG));
        }
    }

    if (first && last)
    {
        if (Count)
            *Count = PtrToUlong(PTR_SUB_OFFSET(last + debugEntrySize, first)) / debugEntrySize;
        if (Address)
            *Address = first;

        return TRUE;
    }

    return FALSE;
}

VOID PvEnumerateCrtInitializers(
    _In_ HWND WindowHandle,
    _In_ HWND ListViewHandle
    )
{
    ULONG64 baseAddress = ULLONG_MAX;
    ULONG_PTR array = 0;
    ULONG count = 0;
    ULONG index = 0;
    ULONG size = 0;

    if (!PvGetCRTFunctionSegment(&count, &array))
        return;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        baseAddress = (ULONG64)PvMappedImage.NtHeaders32->OptionalHeader.ImageBase;
        size = sizeof(ULONG);
    }
    else
    {
        baseAddress = (ULONG64)PvMappedImage.NtHeaders->OptionalHeader.ImageBase;
        size = sizeof(ULONGLONG);
    }

    for (ULONG i = 0; i < count; i++)
    {
        PPH_STRING initSymbol = NULL;
        PPH_STRING initSymbolName = NULL;
        PH_SYMBOL_RESOLVE_LEVEL symbolResolveLevel = PhsrlInvalid;
        ULONG64 displacement = 0;
        INT lvItemIndex;
        WCHAR value[PH_INT64_STR_LEN_1];

        PhPrintUInt64(value, ++index);
        lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);
        PhPrintPointer(value, PTR_ADD_OFFSET(array, UInt32x32To64(i, size)));
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, value);

        initSymbol = PhGetSymbolFromAddress(
            PvSymbolProvider,
            (ULONG64)PTR_ADD_OFFSET(baseAddress, PTR_ADD_OFFSET(array, UInt32x32To64(i, size))),
            &symbolResolveLevel,
            NULL,
            &initSymbolName,
            &displacement
            );

        if (initSymbol)
        {
            switch (symbolResolveLevel)
            {
            case PhsrlFunction:
                {
                    if (displacement)
                    {
                        PhSetListViewSubItem(
                            ListViewHandle,
                            lvItemIndex,
                            2,
                            PhaFormatString(L"%s+0x%llx", initSymbolName->Buffer, displacement)->Buffer
                            );
                    }
                    else
                    {
                        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, initSymbolName->Buffer);
                    }
                }
                break;
            case PhsrlModule:
            case PhsrlAddress:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, initSymbol->Buffer);
                break;
            default:
            case PhsrlInvalid:
                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, L"(unnamed)");
                break;
            }

            if (initSymbolName)
                PhDereferenceObject(initSymbolName);
            PhDereferenceObject(initSymbol);
        }
    }

    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeDebugCrtDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_CRT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_CRT_CONTEXT));
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
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"RVA");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Symbol");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageDebugCrtListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvEnumerateCrtInitializers(hwndDlg, context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageDebugCrtListViewColumns", context->ListViewHandle);

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


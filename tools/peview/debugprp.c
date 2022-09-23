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

typedef struct _PVP_PE_DEBUG_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_DEBUG_CONTEXT, *PPVP_PE_DEBUG_CONTEXT;

PWSTR PvpGetDebugTypeString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case IMAGE_DEBUG_TYPE_COFF:
        return L"COFF";
    case IMAGE_DEBUG_TYPE_CODEVIEW:
        return L"CODEVIEW";
    case IMAGE_DEBUG_TYPE_FPO:
        return L"FPO";
    case IMAGE_DEBUG_TYPE_MISC:
        return L"MISC";
    case IMAGE_DEBUG_TYPE_EXCEPTION:
        return L"EXCEPTION";
    case IMAGE_DEBUG_TYPE_FIXUP:
        return L"FIXUP";
    case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
        return L"OMAP_TO_SRC";
    case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
        return L"OMAP_FROM_SRC";
    case IMAGE_DEBUG_TYPE_BORLAND:
        return L"BORLAND";
    case IMAGE_DEBUG_TYPE_RESERVED10: // coreclr
        return L"RESERVED10";
    case IMAGE_DEBUG_TYPE_CLSID:
        return L"CLSID";
    case IMAGE_DEBUG_TYPE_VC_FEATURE:
        return L"VC_FEATURE";
    case IMAGE_DEBUG_TYPE_POGO:
        return L"POGO";
    case IMAGE_DEBUG_TYPE_ILTCG:
        return L"ILTCG";
    case IMAGE_DEBUG_TYPE_MPX:
        return L"MPX";
    case IMAGE_DEBUG_TYPE_REPRO:
        return L"REPRO";
    case IMAGE_DEBUG_TYPE_EMBEDDEDPORTABLEPDB:
        return L"EMBEDDED_PDB";
    // Note: missing 18.
    case IMAGE_DEBUG_TYPE_PDBCHECKSUM:
        return L"PDB_CHECKSUM";
    case IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS:
        return L"EX_DLLCHARACTERISTICS";
    }

    return PhaFormatString(L"%lu", Type)->Buffer;
}

VOID PvpSetImagelistD(
    _In_ PPVP_PE_DEBUG_CONTEXT context
    )
{
    HIMAGELIST listViewImageList;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(context->WindowHandle);

    listViewImageList = PhImageListCreate(
        2,
        PhGetDpi(20, dpiValue),
        ILC_MASK | ILC_COLOR32,
        1,
        1
        );

    if (listViewImageList)
        ListView_SetImageList(context->ListViewHandle, listViewImageList, LVSIL_SMALL);

}

INT_PTR CALLBACK PvpPeDebugDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_DEBUG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_DEBUG_CONTEXT));
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
            PH_MAPPED_IMAGE_DEBUG debug;
            PH_IMAGE_DEBUG_ENTRY entry;
            ULONG count = 0;
            ULONG i;
            INT lvItemIndex;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"RVA (start)");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"RVA (end)");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Size");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"Hash");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageDebugListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvpSetImagelistD(context);

            if (NT_SUCCESS(PhGetMappedImageDebug(&PvMappedImage, &debug)))
            {
                for (i = 0; i < debug.NumberOfEntries; i++)
                {
                    WCHAR value[PH_INT64_STR_LEN_1];

                    entry = debug.DebugEntries[i];

                    PhPrintUInt32(value, ++count);
                    lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, value, NULL);
                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, PvpGetDebugTypeString(entry.Type));
                    PhPrintPointer(value, UlongToPtr(entry.AddressOfRawData));
                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, value);
                    PhPrintPointer(value, PTR_ADD_OFFSET(entry.AddressOfRawData, entry.SizeOfData));
                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, value);
                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 4, PhaFormatSize(entry.SizeOfData, ULONG_MAX)->Buffer);

                    if (entry.AddressOfRawData && entry.SizeOfData)
                    {
                        __try
                        {
                            PVOID imageSectionData;
                            PH_HASH_CONTEXT hashContext;
                            PPH_STRING hashString;
                            UCHAR hash[32];

                            if (imageSectionData = PhMappedImageRvaToVa(&PvMappedImage, entry.AddressOfRawData, NULL))
                            {
                                PhInitializeHash(&hashContext, Md5HashAlgorithm); // PhGetIntegerSetting(L"HashAlgorithm")
                                PhUpdateHash(&hashContext, imageSectionData, entry.SizeOfData);

                                if (PhFinalHash(&hashContext, hash, 16, NULL))
                                {
                                    hashString = PhBufferToHexString(hash, 16);
                                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 5, hashString->Buffer);
                                    PhDereferenceObject(hashString);
                                }
                            }
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            PPH_STRING message;

                            //message = PH_AUTO(PhGetNtMessage(GetExceptionCode()));
                            message = PH_AUTO(PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode()))); // WIN32_FROM_NTSTATUS

                            PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 5, PhGetStringOrEmpty(message));
                        }
                    }
                }

                PhFree(debug.DebugEntries);
            }

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageDebugListViewColumns", context->ListViewHandle);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PvpSetImagelistD(context);
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

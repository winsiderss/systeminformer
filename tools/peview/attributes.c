/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2018-2022
 *
 */

#include <peview.h>

typedef struct _PVP_PE_ATTRIBUTES_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_ATTRIBUTES_CONTEXT, *PPVP_PE_ATTRIBUTES_CONTEXT;

typedef struct _PV_EA_CALLBACK
{
    HWND ListViewHandle;
    ULONG Count;
} PV_EA_CALLBACK, *PPV_EA_CALLBACK;

BOOLEAN NTAPI PvpEnumFileAttributesCallback(
    _In_ PFILE_FULL_EA_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PPV_EA_CALLBACK context = Context;
    PPH_STRING attributeName;
    INT lvItemIndex;
    WCHAR number[PH_INT32_STR_LEN_1];

    if (!context)
        return TRUE;
    if (Information->EaNameLength == 0)
        return TRUE;

    PhPrintUInt32(number, ++context->Count);
    lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, number, NULL);

    attributeName = PhZeroExtendToUtf16Ex(Information->EaName, Information->EaNameLength);
    PhSetListViewSubItem(
        context->ListViewHandle,
        lvItemIndex,
        1,
        attributeName->Buffer
        );
    PhDereferenceObject(attributeName);

    PhSetListViewSubItem(
        context->ListViewHandle,
        lvItemIndex,
        2,
        PhaFormatSize(Information->EaValueLength, ULONG_MAX)->Buffer
        );

    return TRUE;
}

VOID PvEnumerateFileExtendedAttributes(
    _In_ HWND ListViewHandle
    )
{
    HANDLE fileHandle;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ATTRIBUTES | FILE_READ_EA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        PV_EA_CALLBACK context;

        context.ListViewHandle = ListViewHandle;
        context.Count = 0;

        PhEnumFileExtendedAttributes(fileHandle, PvpEnumFileAttributesCallback, &context);

        NtClose(fileHandle);
    }

    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

VOID PvpSetImagelist(
    _In_ PPVP_PE_ATTRIBUTES_CONTEXT context
    )
{
    HIMAGELIST listViewImageList;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(context->WindowHandle);

    listViewImageList = PhImageListCreate (
        2,
        PhGetDpi(20, dpiValue),
        ILC_MASK | ILC_COLOR,
        1,
        1
        );

    if (listViewImageList)
        ListView_SetImageList(context->ListViewHandle, listViewImageList, LVSIL_SMALL);
}

INT_PTR CALLBACK PvpPeExtendedAttributesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_ATTRIBUTES_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_ATTRIBUTES_CONTEXT));
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
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageAttributesListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvpSetImagelist(context);

            PvEnumerateFileExtendedAttributes(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageAttributesListViewColumns", context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PvpSetImagelist(context);
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
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PvGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Delete", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"&Copy", NULL, NULL), ULONG_MAX);
                    PvInsertCopyListViewEMenuItem(menu, USHRT_MAX, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PvHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case 1:
                                {
                                    NTSTATUS status;
                                    HANDLE fileHandle;
                                    PPH_STRING nameUtf = NULL;
                                    PPH_BYTES nameAnsi = NULL;
                                    INT index;

                                    if ((index = PhFindListViewItemByFlags(context->ListViewHandle, -1, LVNI_SELECTED)) != -1)
                                    {
                                        nameUtf = PhGetListViewItemText(context->ListViewHandle, index, 1);
                                    }

                                    if (PhIsNullOrEmptyString(nameUtf))
                                        break;

                                    nameAnsi = PhConvertUtf16ToUtf8Ex(nameUtf->Buffer, nameUtf->Length);

                                    if (NT_SUCCESS(status = PhCreateFileWin32(
                                        &fileHandle,
                                        PhGetString(PvFileName),
                                        FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | SYNCHRONIZE,
                                        FILE_ATTRIBUTE_NORMAL,
                                        FILE_SHARE_WRITE,
                                        FILE_OPEN,
                                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                                        )))
                                    {
                                        status = PhSetFileExtendedAttributes(fileHandle, &nameAnsi->br, NULL);
                                        NtClose(fileHandle);
                                    }

                                    if (NT_SUCCESS(status))
                                    {
                                        PvEnumerateFileExtendedAttributes(context->ListViewHandle);
                                    }
                                    else
                                    {
                                        PhShowStatus(hwndDlg, L"Unable to remove attribute.", status, 0);
                                    }

                                    PhClearReference(&nameUtf);
                                    PhClearReference(&nameAnsi);
                                }
                                break;
                            case USHRT_MAX:
                                {
                                    PvCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
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

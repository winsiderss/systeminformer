/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2023
 *
 */

#include "exttools.h"
#include "poolmon.h"

NTSTATUS EtEnumBigPoolTable(
    _Out_ PVOID* Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemBigPoolInformation,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemBigPoolInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
}

VOID EtUpdateBigPoolTable(
    _Inout_ PBIGPOOLTAG_CONTEXT Context
    )
{
    PSYSTEM_BIGPOOL_INFORMATION bigPoolTable;
    ULONG i;

    if (!NT_SUCCESS(EtEnumBigPoolTable(&bigPoolTable)))
        return;

    for (i = 0; i < bigPoolTable->Count; i++)
    {
        INT itemIndex;
        SYSTEM_BIGPOOL_ENTRY poolTagInfo;
        WCHAR virtualAddressString[PH_PTR_STR_LEN_1] = L"";

        poolTagInfo = bigPoolTable->AllocatedInfo[i];

        if (poolTagInfo.TagUlong != Context->TagUlong)
            continue;

        PhPrintPointer(virtualAddressString, poolTagInfo.VirtualAddress);
        itemIndex = PhAddListViewItem(
            Context->ListViewHandle,
            MAXINT,
            virtualAddressString,
            NULL
            );

        PhSetListViewSubItem(
            Context->ListViewHandle,
            itemIndex,
            1,
            PhaFormatSize(poolTagInfo.SizeInBytes, ULONG_MAX)->Buffer
            );

        if (poolTagInfo.NonPaged)
        {
            PhSetListViewSubItem(
                Context->ListViewHandle,
                itemIndex,
                2,
                L"Yes"
                );
        }
        else
        {
            PhSetListViewSubItem(
                Context->ListViewHandle,
                itemIndex,
                2,
                L"No"
                );
        }
    }

    PhFree(bigPoolTable);
}

INT_PTR CALLBACK EtBigPoolMonDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PBIGPOOLTAG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(BIGPOOLTAG_CONTEXT));
        context->TagUlong = ((PPOOL_ITEM)lParam)->TagUlong;
        PhZeroExtendToUtf16Buffer(context->Tag, sizeof(context->Tag), context->TagString);

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_BIGPOOLLIST);

            PhSetApplicationWindowIcon(hwndDlg);

            PhSetWindowText(hwndDlg, PhaFormatString(L"Large Allocations (%s)", context->TagString)->Buffer);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 150, L"Address");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Size");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"NonPaged");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhGetIntegerPairSetting(SETTING_NAME_BIGPOOL_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_BIGPOOL_WINDOW_POSITION, SETTING_NAME_BIGPOOL_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, NULL);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            EtUpdateBigPoolTable(context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhSaveWindowPlacementToSetting(SETTING_NAME_BIGPOOL_WINDOW_POSITION, SETTING_NAME_BIGPOOL_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_REFRESH:
                {
                    ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                    ListView_DeleteAllItems(context->ListViewHandle);

                    EtUpdateBigPoolTable(context);

                    ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);

            switch (hdr->code)
            {
            case NM_RCLICK:
                {
                    if (hdr->hwndFrom == context->ListViewHandle)
                    {
                        POINT point;
                        PPH_EMENU menu;
                        ULONG numberOfItems;
                        PVOID* listviewItems;
                        PPH_EMENU_ITEM selectedItem;

                        PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                        if (numberOfItems == 0)
                            break;

                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"&Copy", NULL, NULL), ULONG_MAX);
                        PhInsertCopyListViewEMenuItem(menu, USHRT_MAX, context->ListViewHandle);

                        GetCursorPos(&point);
                        selectedItem = PhShowEMenu(
                            menu,
                            context->ListViewHandle,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            point.x,
                            point.y
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
                        {
                            if (PhHandleCopyListViewEMenuItem(selectedItem))
                                break;

                            switch (selectedItem->Id)
                            {
                            case USHRT_MAX:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }

                        PhDestroyEMenu(menu);
                    }
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID EtShowBigPoolDialog(
    _In_ PPOOL_ITEM PoolItem
    )
{
    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_BIGPOOL),
        NULL,
        EtBigPoolMonDlgProc,
        PoolItem
        );
}

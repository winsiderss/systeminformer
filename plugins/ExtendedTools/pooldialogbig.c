/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016
 *
 */

#include "exttools.h"
#include "poolmon.h"

VOID UpdateBigPoolTable(
    _Inout_ PBIGPOOLTAG_CONTEXT Context
    )
{
    PSYSTEM_BIGPOOL_INFORMATION bigPoolTable;
    ULONG i;
    
    if (!NT_SUCCESS(EnumBigPoolTable(&bigPoolTable)))
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
            Context->ListviewHandle,
            MAXINT,
            virtualAddressString,
            NULL
            );

        PhSetListViewSubItem(
            Context->ListviewHandle,
            itemIndex,
            1,
            PhaFormatSize(poolTagInfo.SizeInBytes, -1)->Buffer
            );

        if (poolTagInfo.NonPaged)
        {
            PhSetListViewSubItem(
                Context->ListviewHandle,
                itemIndex,
                2,
                L"Yes"
                );
        }
        else
        {
            PhSetListViewSubItem(
                Context->ListviewHandle,
                itemIndex,
                2,
                L"No"
                );
        }
    }

    PhFree(bigPoolTable);
}

INT_PTR CALLBACK BigPoolMonDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PBIGPOOLTAG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(BIGPOOLTAG_CONTEXT));
        memset(context, 0, sizeof(BIGPOOLTAG_CONTEXT));

        context->TagUlong = ((PPOOL_ITEM)lParam)->TagUlong;
        PhZeroExtendToUtf16Buffer(context->Tag, sizeof(context->Tag), context->TagString);

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListviewHandle = GetDlgItem(hwndDlg, IDC_BIGPOOLLIST);

            PhCenterWindow(hwndDlg, NULL);

            PhSetWindowText(hwndDlg, PhaFormatString(L"Large Allocations (%s)", context->TagString)->Buffer);

            PhRegisterDialog(hwndDlg);
            PhSetListViewStyle(context->ListviewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListviewHandle, L"explorer");
            PhAddListViewColumn(context->ListviewHandle, 0, 0, 0, LVCFMT_LEFT, 150, L"Address");
            PhAddListViewColumn(context->ListviewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Size");
            PhAddListViewColumn(context->ListviewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"NonPaged");
            PhSetExtendedListView(context->ListviewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListviewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            //PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            UpdateBigPoolTable(context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_DESTROY:
        {
            //PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhUnregisterDialog(hwndDlg);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID ShowBigPoolDialog(
    _In_ PPOOL_ITEM PoolItem
    )
{
    DialogBoxParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_BIGPOOL),
        NULL,
        BigPoolMonDlgProc,
        (LPARAM)PoolItem
        );
}

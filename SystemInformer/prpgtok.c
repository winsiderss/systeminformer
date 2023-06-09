/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2018-2022
 *
 */

#include <phapp.h>
#include <procprp.h>
#include <procprv.h>

NTSTATUS NTAPI PhpOpenProcessTokenForPage(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (!PH_IS_REAL_PROCESS_ID(Context))
        return STATUS_UNSUCCESSFUL;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION,
        (HANDLE)Context
        )))
        return status;

    if (!NT_SUCCESS(status = PhOpenProcessToken(
        processHandle,
        DesiredAccess | TOKEN_READ | TOKEN_ADJUST_DEFAULT | READ_CONTROL,
        Handle
        )))
    {
        if (!NT_SUCCESS(status = PhOpenProcessToken(
            processHandle,
            DesiredAccess | TOKEN_READ | TOKEN_ADJUST_DEFAULT,
            Handle
            )))
        {
            status = PhOpenProcessToken(
                processHandle,
                DesiredAccess,
                Handle
                );
        }
    }

    NtClose(processHandle);

    return status;
}

INT_PTR CALLBACK PhpProcessTokenHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_DESTROY:
        {
            if (PhGetWindowContext(hwndDlg, 0xD))
                PhRemoveWindowContext(hwndDlg, 0xD);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!PhGetWindowContext(hwndDlg, 0xD)) // LayoutInitialized
            {
                PPH_LAYOUT_ITEM dialogItem;

                // This is a big violation of abstraction...

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_USER), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_USERSID), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VIRTUALIZED), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_GROUPS), dialogItem, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_DEFAULTPERM), dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PERMISSIONS), dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_INTEGRITY), dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_ADVANCED), dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

                PhDoPropPageLayout(hwndDlg);

                PhSetWindowContext(hwndDlg, 0xD, UlongToPtr(TRUE));
            }
        }
        break;
    }

    return FALSE;
}

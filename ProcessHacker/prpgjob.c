/*
 * Process Hacker -
 *   Process properties: Job page
 *
 * Copyright (C) 2009-2016 wj32
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

#include <phapp.h>
#include <procprp.h>
#include <procprpp.h>

#include <kphuser.h>

NTSTATUS NTAPI PhpOpenProcessJobForPage(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE jobHandle = NULL;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION,
        (HANDLE)Context
        )))
        return status;

    status = KphOpenProcessJob(processHandle, DesiredAccess, &jobHandle);
    NtClose(processHandle);

    if (NT_SUCCESS(status) && status != STATUS_PROCESS_NOT_IN_JOB && jobHandle)
    {
        *Handle = jobHandle;
    }
    else if (NT_SUCCESS(status))
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

INT_PTR CALLBACK PhpProcessJobHookProc(
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
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT)) // LayoutInitialized
            {
                PPH_LAYOUT_ITEM dialogItem;

                // This is a big violation of abstraction...

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_NAME),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_TERMINATE),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCESSES),
                    dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_ADD),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIMITS),
                    dialogItem, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_ADVANCED),
                    dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

                PhDoPropPageLayout(hwndDlg);

                PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, UlongToPtr(TRUE));
            }
        }
        break;
    }

    return FALSE;
}

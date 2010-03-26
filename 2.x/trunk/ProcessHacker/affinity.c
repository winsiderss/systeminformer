/*
 * Process Hacker - 
 *   process affinity editor
 * 
 * Copyright (C) 2010 wj32
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
#include <windowsx.h>

INT_PTR CALLBACK PhpProcessAffinityDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowProcessAffinityDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        (LPARAM)ProcessItem
        );
}

static INT_PTR CALLBACK PhpProcessAffinityDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;
            PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)lParam;
            HANDLE processHandle;
            PROCESS_BASIC_INFORMATION basicInfo;
            SYSTEM_BASIC_INFORMATION systemBasicInfo;
            ULONG i;

            SetProp(hwndDlg, L"ProcessItem", (HANDLE)processItem);
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            if (NT_SUCCESS(status = PhOpenProcess(
                &processHandle,
                ProcessQueryAccess,
                processItem->ProcessId
                )))
            {
                status = PhGetProcessBasicInformation(processHandle, &basicInfo);

                if (NT_SUCCESS(status))
                {
                    status = NtQuerySystemInformation(
                        SystemBasicInformation,
                        &systemBasicInfo,
                        sizeof(SYSTEM_BASIC_INFORMATION),
                        NULL
                        );
                }

                NtClose(processHandle);
            }

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(hwndDlg, L"Unable to retrieve the process' affinity", status, 0);
                EndDialog(hwndDlg, IDCANCEL);
            }

            // Disable the CPU checkboxes which aren't part of the system affinity mask, 
            // and check the CPU checkboxes which are part of the process affinity mask.

            for (i = 0; i < 32; i++)
            {
                if ((systemBasicInfo.ActiveProcessorsAffinityMask >> i) & 0x1)
                {
                    if ((basicInfo.AffinityMask >> i) & 0x1)
                    {
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_CPU0 + i), BST_CHECKED);
                    }
                }
                else
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_CPU0 + i), FALSE);
                }
            }
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"ProcessItem");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    NTSTATUS status;
                    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)GetProp(hwndDlg, L"ProcessItem");
                    ULONG i;
                    ULONG_PTR affinityMask;
                    HANDLE processHandle;

                    // Work out the affinity mask.

                    affinityMask = 0;

                    for (i = 0; i < 32; i++)
                    {
                        if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_CPU0 + i)) == BST_CHECKED)
                            affinityMask |= 1 << i;
                    }

                    // Open the process and set the affinity mask.

                    if (NT_SUCCESS(status = PhOpenProcess(
                        &processHandle,
                        PROCESS_SET_INFORMATION,
                        processItem->ProcessId
                        )))
                    {
                        status = PhSetProcessAffinityMask(processHandle, affinityMask);
                        NtClose(processHandle);
                    }

                    if (NT_SUCCESS(status))
                        EndDialog(hwndDlg, IDOK);
                    else
                        PhShowStatus(hwndDlg, L"Unable to set the process' affinity", status, 0);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

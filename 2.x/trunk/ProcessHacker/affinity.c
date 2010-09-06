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

typedef struct _AFFINITY_DIALOG_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    ULONG_PTR AffinityMask;
    ULONG_PTR NewAffinityMask;
} AFFINITY_DIALOG_CONTEXT, *PAFFINITY_DIALOG_CONTEXT;

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
    AFFINITY_DIALOG_CONTEXT context;

    context.ProcessItem = ProcessItem;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        (LPARAM)&context
        );
}

BOOLEAN PhShowProcessAffinityDialog2(
    __in HWND ParentWindowHandle,
    __in ULONG_PTR AffinityMask,
    __out PULONG_PTR NewAffinityMask
    )
{
    AFFINITY_DIALOG_CONTEXT context;

    context.ProcessItem = NULL;
    context.AffinityMask = AffinityMask;

    if (DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        (LPARAM)&context
        ) == IDOK)
    {
        *NewAffinityMask = context.NewAffinityMask;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
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
            PAFFINITY_DIALOG_CONTEXT context = (PAFFINITY_DIALOG_CONTEXT)lParam;
            HANDLE processHandle;
            PROCESS_BASIC_INFORMATION basicInfo;
            SYSTEM_BASIC_INFORMATION systemBasicInfo;
            ULONG_PTR processAffinityMask;
            ULONG i;

            SetProp(hwndDlg, L"Context", (HANDLE)context);
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            if (context->ProcessItem)
            {
                if (NT_SUCCESS(status = PhOpenProcess(
                    &processHandle,
                    ProcessQueryAccess,
                    context->ProcessItem->ProcessId
                    )))
                {
                    status = PhGetProcessBasicInformation(processHandle, &basicInfo);

                    if (NT_SUCCESS(status))
                        processAffinityMask = basicInfo.AffinityMask;

                    NtClose(processHandle);
                }
            }
            else
            {
                processAffinityMask = context->AffinityMask;
                status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(status))
            {
                status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &systemBasicInfo,
                    sizeof(SYSTEM_BASIC_INFORMATION),
                    NULL
                    );
            }

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(hwndDlg, L"Unable to retrieve the process' affinity", status, 0);
                EndDialog(hwndDlg, IDCANCEL);
                break;
            }

            // Disable the CPU checkboxes which aren't part of the system affinity mask, 
            // and check the CPU checkboxes which are part of the process affinity mask.

            for (i = 0; i < 32; i++)
            {
                if ((systemBasicInfo.ActiveProcessorsAffinityMask >> i) & 0x1)
                {
                    if ((processAffinityMask >> i) & 0x1)
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
                    PAFFINITY_DIALOG_CONTEXT context = (PAFFINITY_DIALOG_CONTEXT)GetProp(hwndDlg, L"Context");
                    ULONG i;
                    ULONG_PTR affinityMask;
                    HANDLE processHandle;

                    // Work out the affinity mask.

                    affinityMask = 0;

                    for (i = 0; i < 32; i++)
                    {
                        if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_CPU0 + i)) == BST_CHECKED)
                            affinityMask |= (ULONG_PTR)1 << i;
                    }

                    if (context->ProcessItem)
                    {
                        // Open the process and set the affinity mask.

                        if (NT_SUCCESS(status = PhOpenProcess(
                            &processHandle,
                            PROCESS_SET_INFORMATION,
                            context->ProcessItem->ProcessId
                            )))
                        {
                            status = PhSetProcessAffinityMask(processHandle, affinityMask);
                            NtClose(processHandle);
                        }
                    }
                    else
                    {
                        context->NewAffinityMask = affinityMask;
                        status = STATUS_SUCCESS;
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

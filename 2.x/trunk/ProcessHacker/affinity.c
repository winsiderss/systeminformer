/*
 * Process Hacker -
 *   process affinity editor
 *
 * Copyright (C) 2010-2015 wj32
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

/*
 * The affinity dialog was originally created to support the modification
 * of process affinity masks, but now supports modifying thread affinity
 * and generic masks.
 */

#include <phapp.h>
#include <windowsx.h>

typedef struct _AFFINITY_DIALOG_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    PPH_THREAD_ITEM ThreadItem;
    ULONG_PTR AffinityMask;
    ULONG_PTR NewAffinityMask;
} AFFINITY_DIALOG_CONTEXT, *PAFFINITY_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpProcessAffinityDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowProcessAffinityDialog(
    _In_ HWND ParentWindowHandle,
    _In_opt_ PPH_PROCESS_ITEM ProcessItem,
    _In_opt_ PPH_THREAD_ITEM ThreadItem
    )
{
    AFFINITY_DIALOG_CONTEXT context;

    assert(!!ProcessItem != !!ThreadItem); // make sure we have one and not the other

    context.ProcessItem = ProcessItem;
    context.ThreadItem = ThreadItem;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_AFFINITY),
        ParentWindowHandle,
        PhpProcessAffinityDlgProc,
        (LPARAM)&context
        );
}

BOOLEAN PhShowProcessAffinityDialog2(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG_PTR AffinityMask,
    _Out_ PULONG_PTR NewAffinityMask
    )
{
    AFFINITY_DIALOG_CONTEXT context;

    context.ProcessItem = NULL;
    context.ThreadItem = NULL;
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;
            PAFFINITY_DIALOG_CONTEXT context = (PAFFINITY_DIALOG_CONTEXT)lParam;
            SYSTEM_BASIC_INFORMATION systemBasicInfo;
            ULONG_PTR systemAffinityMask;
            ULONG_PTR affinityMask;
            ULONG i;

            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            systemAffinityMask = 0;

            if (context->ProcessItem)
            {
                HANDLE processHandle;
                PROCESS_BASIC_INFORMATION basicInfo;

                if (NT_SUCCESS(status = PhOpenProcess(
                    &processHandle,
                    ProcessQueryAccess,
                    context->ProcessItem->ProcessId
                    )))
                {
                    status = PhGetProcessBasicInformation(processHandle, &basicInfo);

                    if (NT_SUCCESS(status))
                        affinityMask = basicInfo.AffinityMask;

                    NtClose(processHandle);
                }
            }
            else if (context->ThreadItem)
            {
                HANDLE threadHandle;
                THREAD_BASIC_INFORMATION basicInfo;
                HANDLE processHandle;
                PROCESS_BASIC_INFORMATION processBasicInfo;

                if (NT_SUCCESS(status = PhOpenThread(
                    &threadHandle,
                    ThreadQueryAccess,
                    context->ThreadItem->ThreadId
                    )))
                {
                    status = PhGetThreadBasicInformation(threadHandle, &basicInfo);

                    if (NT_SUCCESS(status))
                    {
                        affinityMask = basicInfo.AffinityMask;

                        // A thread's affinity mask is restricted by the process affinity mask,
                        // so use that as the system affinity mask.

                        if (NT_SUCCESS(PhOpenProcess(
                            &processHandle,
                            ProcessQueryAccess,
                            basicInfo.ClientId.UniqueProcess
                            )))
                        {
                            if (NT_SUCCESS(PhGetProcessBasicInformation(processHandle, &processBasicInfo)))
                                systemAffinityMask = processBasicInfo.AffinityMask;

                            NtClose(processHandle);
                        }
                    }

                    NtClose(threadHandle);
                }
            }
            else
            {
                affinityMask = context->AffinityMask;
                status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(status) && systemAffinityMask == 0)
            {
                status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &systemBasicInfo,
                    sizeof(SYSTEM_BASIC_INFORMATION),
                    NULL
                    );

                if (NT_SUCCESS(status))
                    systemAffinityMask = systemBasicInfo.ActiveProcessorsAffinityMask;
            }

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(hwndDlg, L"Unable to retrieve the affinity", status, 0);
                EndDialog(hwndDlg, IDCANCEL);
                break;
            }

            // Disable the CPU checkboxes which aren't part of the system affinity mask,
            // and check the CPU checkboxes which are part of the affinity mask.

            for (i = 0; i < 8 * 8; i++)
            {
                if ((i < sizeof(ULONG_PTR) * 8) && ((systemAffinityMask >> i) & 0x1))
                {
                    if ((affinityMask >> i) & 0x1)
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
            RemoveProp(hwndDlg, PhMakeContextAtom());
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
                    PAFFINITY_DIALOG_CONTEXT context = (PAFFINITY_DIALOG_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());
                    ULONG i;
                    ULONG_PTR affinityMask;

                    // Work out the affinity mask.

                    affinityMask = 0;

                    for (i = 0; i < sizeof(ULONG_PTR) * 8; i++)
                    {
                        if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_CPU0 + i)) == BST_CHECKED)
                            affinityMask |= (ULONG_PTR)1 << i;
                    }

                    if (context->ProcessItem)
                    {
                        HANDLE processHandle;

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
                    else if (context->ThreadItem)
                    {
                        HANDLE threadHandle;

                        if (NT_SUCCESS(status = PhOpenThread(
                            &threadHandle,
                            ThreadSetAccess,
                            context->ThreadItem->ThreadId
                            )))
                        {
                            status = PhSetThreadAffinityMask(threadHandle, affinityMask);
                            NtClose(threadHandle);
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
                        PhShowStatus(hwndDlg, L"Unable to set the affinity", status, 0);
                }
                break;
            case IDC_SELECTALL:
            case IDC_DESELECTALL:
                {
                    ULONG i;

                    for (i = 0; i < sizeof(ULONG_PTR) * 8; i++)
                    {
                        HWND checkBox = GetDlgItem(hwndDlg, IDC_CPU0 + i);

                        if (IsWindowEnabled(checkBox))
                            Button_SetCheck(checkBox, LOWORD(wParam) == IDC_SELECTALL ? BST_CHECKED : BST_UNCHECKED);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

/*
 * Process Hacker -
 *   memory protection window
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

typedef struct _MEMORY_PROTECT_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    PPH_MEMORY_ITEM MemoryItem;
} MEMORY_PROTECT_CONTEXT, *PMEMORY_PROTECT_CONTEXT;

INT_PTR CALLBACK PhpMemoryProtectDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowMemoryProtectDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_MEMORY_ITEM MemoryItem
    )
{
    MEMORY_PROTECT_CONTEXT context;

    context.ProcessItem = ProcessItem;
    context.MemoryItem = MemoryItem;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MEMPROTECT),
        ParentWindowHandle,
        PhpMemoryProtectDlgProc,
        (LPARAM)&context
        );
}

static INT_PTR CALLBACK PhpMemoryProtectDlgProc(
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
            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)lParam);

            SetDlgItemText(hwndDlg, IDC_INTRO,
                L"Possible values:\r\n"
                L"\r\n"
                L"0x01 - PAGE_NOACCESS\r\n"
                L"0x02 - PAGE_READONLY\r\n"
                L"0x04 - PAGE_READWRITE\r\n"
                L"0x08 - PAGE_WRITECOPY\r\n"
                L"0x10 - PAGE_EXECUTE\r\n"
                L"0x20 - PAGE_EXECUTE_READ\r\n"
                L"0x40 - PAGE_EXECUTE_READWRITE\r\n"
                L"0x80 - PAGE_EXECUTE_WRITECOPY\r\n"
                L"Modifiers:\r\n"
                L"0x100 - PAGE_GUARD\r\n"
                L"0x200 - PAGE_NOCACHE\r\n"
                L"0x400 - PAGE_WRITECOMBINE\r\n"
                );

            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwndDlg, IDC_VALUE), TRUE);
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
                    PMEMORY_PROTECT_CONTEXT context = (PMEMORY_PROTECT_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());
                    HANDLE processHandle;
                    ULONG64 protect;

                    PhStringToInteger64(&PhaGetDlgItemText(hwndDlg, IDC_VALUE)->sr, 0, &protect);

                    if (NT_SUCCESS(status = PhOpenProcess(
                        &processHandle,
                        PROCESS_VM_OPERATION,
                        context->ProcessItem->ProcessId
                        )))
                    {
                        PVOID baseAddress;
                        SIZE_T regionSize;
                        ULONG oldProtect;

                        baseAddress = context->MemoryItem->BaseAddress;
                        regionSize = context->MemoryItem->RegionSize;

                        status = NtProtectVirtualMemory(
                            processHandle,
                            &baseAddress,
                            &regionSize,
                            (ULONG)protect,
                            &oldProtect
                            );

                        if (NT_SUCCESS(status))
                            context->MemoryItem->Protect = (ULONG)protect;
                    }

                    if (NT_SUCCESS(status))
                    {
                        EndDialog(hwndDlg, IDOK);
                    }
                    else
                    {
                        PhShowStatus(hwndDlg, L"Unable to change memory protection", status, 0);
                        SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwndDlg, IDC_VALUE), TRUE);
                        Edit_SetSel(GetDlgItem(hwndDlg, IDC_VALUE), 0, -1);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

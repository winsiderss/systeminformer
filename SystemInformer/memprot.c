/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *
 */

#include <phapp.h>
#include <memprv.h>
#include <procprv.h>

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
            PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, (PVOID)lParam);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetDialogItemText(hwndDlg, IDC_INTRO,
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

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDC_VALUE));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    NTSTATUS status;
                    PMEMORY_PROTECT_CONTEXT context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
                        PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDC_VALUE));
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

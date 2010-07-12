/*
 * Process Hacker - 
 *   memory editor window
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
#include <hexedit.h>
#include <windowsx.h>

typedef struct _MEMORY_EDITOR_CONTEXT
{
    PH_AVL_LINKS Links;
    union
    {
        struct
        {
            HANDLE ProcessId;
            PVOID BaseAddress;
            SIZE_T RegionSize;
        };
        ULONG_PTR Key[3];
    };
    HANDLE ProcessHandle;
    HWND WindowHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    HWND HexEditHandle;
    PUCHAR Buffer;
} MEMORY_EDITOR_CONTEXT, *PMEMORY_EDITOR_CONTEXT;

INT NTAPI PhpMemoryEditorCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    );

INT_PTR CALLBACK PhpMemoryEditorDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

PH_AVL_TREE PhMemoryEditorSet = PH_AVL_TREE_INIT(PhpMemoryEditorCompareFunction);

VOID PhShowMemoryEditorDialog(
    __in HANDLE ProcessId,
    __in PVOID BaseAddress,
    __in SIZE_T RegionSize
    )
{
    PMEMORY_EDITOR_CONTEXT context;
    MEMORY_EDITOR_CONTEXT lookupContext;
    PPH_AVL_LINKS links;

    lookupContext.ProcessId = ProcessId;
    lookupContext.BaseAddress = BaseAddress;
    lookupContext.RegionSize = RegionSize;

    links = PhAvlTreeSearch(&PhMemoryEditorSet, &lookupContext.Links);

    if (!links)
    {
        context = PhAllocate(sizeof(MEMORY_EDITOR_CONTEXT));
        memset(context, 0, sizeof(MEMORY_EDITOR_CONTEXT));

        context->ProcessId = ProcessId;
        context->BaseAddress = BaseAddress;
        context->RegionSize = RegionSize;

        context->WindowHandle = CreateDialogParam(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_MEMEDIT),
            NULL,
            PhpMemoryEditorDlgProc,
            (LPARAM)context
            );
        PhRegisterDialog(context->WindowHandle);
        PhAvlTreeAdd(&PhMemoryEditorSet, &context->Links);

        ShowWindow(context->WindowHandle, SW_SHOW);
    }
    else
    {
        context = CONTAINING_RECORD(links, MEMORY_EDITOR_CONTEXT, Links);

        if (IsIconic(context->WindowHandle))
            ShowWindow(context->WindowHandle, SW_RESTORE);
        else
            SetForegroundWindow(context->WindowHandle);
    }
}

INT NTAPI PhpMemoryEditorCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    )
{
    PMEMORY_EDITOR_CONTEXT context1 = CONTAINING_RECORD(Links1, MEMORY_EDITOR_CONTEXT, Links);
    PMEMORY_EDITOR_CONTEXT context2 = CONTAINING_RECORD(Links2, MEMORY_EDITOR_CONTEXT, Links);

    return memcmp(context1->Key, context2->Key, sizeof(context1->Key));
}

INT_PTR CALLBACK PhpMemoryEditorDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PMEMORY_EDITOR_CONTEXT context;

    if (uMsg != WM_INITDIALOG)
    {
        context = GetProp(hwndDlg, L"Context");
    }
    else
    {
        context = (PMEMORY_EDITOR_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            if (context->RegionSize > 1024 * 1024 * 1024) // 1 GB
            {
                PhShowError(hwndDlg, L"Unable to edit the memory region because it is too large.");
                DestroyWindow(hwndDlg);
                return;
            }

            if (!NT_SUCCESS(status = PhOpenProcess(
                &context->ProcessHandle,
                PROCESS_VM_READ | PROCESS_VM_WRITE,
                context->ProcessId
                )))
            {
                if (!NT_SUCCESS(status = PhOpenProcess(
                    &context->ProcessHandle,
                    PROCESS_VM_READ,
                    context->ProcessId
                    )))
                {
                    PhShowStatus(hwndDlg, L"Unable to open the process", status, 0);
                    DestroyWindow(hwndDlg);
                    return;
                }
            }

            context->Buffer = PhAllocatePage(context->RegionSize, NULL);

            if (!context->Buffer)
            {
                PhShowError(hwndDlg, L"Unable to allocate memory for the buffer.");
                DestroyWindow(hwndDlg);
                return;
            }

            NtReadVirtualMemory(
                context->ProcessHandle,
                context->BaseAddress,
                context->Buffer,
                context->RegionSize,
                NULL
                );

            context->HexEditHandle = PhCreateHexEditControl(hwndDlg, IDC_MEMORY);
            BringWindowToTop(context->HexEditHandle);
            MoveWindow(context->HexEditHandle, 10, 10, 750, 300, FALSE);
            ShowWindow(context->HexEditHandle, SW_SHOW);
            HexEdit_SetBuffer(context->HexEditHandle, context->Buffer, (ULONG)context->RegionSize);
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"Context");
            PhAvlTreeRemove(&PhMemoryEditorSet, &context->Links);
            PhUnregisterDialog(hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->Buffer) PhFreePage(context->Buffer);
            if (context->ProcessHandle) NtClose(context->ProcessHandle);

            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_SAVE:
                {

                }
                break;
            case IDC_WRITE:
                {

                }
                break;
            case IDC_REREAD:
                {

                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 450, 300);
        }
        break;
    }

    return FALSE;
}

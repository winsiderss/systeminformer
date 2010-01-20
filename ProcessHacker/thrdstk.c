/*
 * Process Hacker - 
 *   thread stack viewer
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

#include <phgui.h>

typedef struct THREAD_STACK_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE ThreadHandle;
    HWND ListViewHandle;
    PPH_SYMBOL_PROVIDER SymbolProvider;
} THREAD_STACK_CONTEXT, *PTHREAD_STACK_CONTEXT;

INT_PTR CALLBACK PhpThreadStackDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

NTSTATUS PhpRefreshThreadStack(
    __in PTHREAD_STACK_CONTEXT ThreadStackContext
    );

VOID PhShowThreadStackDialog(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId,
    __in HANDLE ThreadId,
    __in PPH_SYMBOL_PROVIDER SymbolProvider
    )
{
    NTSTATUS status;
    THREAD_STACK_CONTEXT threadStackContext;
    HANDLE threadHandle = NULL;

    // If the user is trying to view a system thread stack 
    // but KProcessHacker is not loaded, show an error message.
    if (ProcessId == SYSTEM_PROCESS_ID && !PhKphHandle)
    {
        PhShowError(ParentWindowHandle, KPH_ERROR_MESSAGE);
        return;
    }

    // We don't want stupid users screwing up the program 
    // by stack walking the current thread or something.
    if (ProcessId == NtCurrentProcessId())
    {
        if (!PhShowConfirmMessage(
            ParentWindowHandle,
            L"inspect",
            L"Process Hacker's threads",
            L"Viewing call stacks of Process Hacker's threads may lead to instability.",
            TRUE
            ))
            return;
    }

    threadStackContext.ProcessId = ProcessId;
    threadStackContext.ThreadId = ThreadId;
    threadStackContext.SymbolProvider = SymbolProvider;

    if (!NT_SUCCESS(status = PhOpenThread(
        &threadHandle,
        THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
        ThreadId
        )))
    {
        if (PhKphHandle)
        {
            status = PhOpenThread(
                &threadHandle,
                ThreadQueryAccess,
                ThreadId
                );
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the thread", status, 0);
        return;
    }

    threadStackContext.ThreadHandle = threadHandle;

    PhReferenceObject(SymbolProvider);

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDSTACK),
        ParentWindowHandle,
        PhpThreadStackDlgProc,
        (LPARAM)&threadStackContext
        );

    if (threadStackContext.ThreadHandle)
        CloseHandle(threadStackContext.ThreadHandle);
} 

static INT_PTR CALLBACK PhpThreadStackDlgProc(      
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
            PTHREAD_STACK_CONTEXT threadStackContext;
            PPH_STRING title;
            HWND lvHandle;
            PPH_LAYOUT_MANAGER layoutManager;

            threadStackContext = (PTHREAD_STACK_CONTEXT)lParam;
            SetProp(hwndDlg, L"Context", (HANDLE)threadStackContext);

            title = PhFormatString(L"Stack - thread %u", (ULONG)threadStackContext->ThreadId);
            SetWindowText(hwndDlg, title->Buffer);
            PhDereferenceObject(title);

            lvHandle = GetDlgItem(hwndDlg, IDC_THRDSTACK_LIST);
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Name");
            ListView_SetExtendedListViewStyleEx(
                lvHandle,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
                -1
                );
            PhSetControlTheme(lvHandle, L"explorer");

            threadStackContext->ListViewHandle = lvHandle;

            layoutManager = PhAllocate(sizeof(PH_LAYOUT_MANAGER));
            PhInitializeLayoutManager(layoutManager, hwndDlg);
            SetProp(hwndDlg, L"LayoutManager", (HANDLE)layoutManager);

            PhAddLayoutItem(layoutManager, lvHandle, NULL,
                PH_ANCHOR_ALL);
            PhAddLayoutItem(layoutManager, GetDlgItem(hwndDlg, IDC_REFRESH),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(layoutManager, GetDlgItem(hwndDlg, IDOK),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhpRefreshThreadStack(threadStackContext);
        }
        break;
    case WM_DESTROY:
        {
            PPH_LAYOUT_MANAGER layoutManager;
            PTHREAD_STACK_CONTEXT threadStackContext;

            layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwndDlg, L"LayoutManager");
            PhDeleteLayoutManager(layoutManager);
            PhFree(layoutManager);

            threadStackContext = (PTHREAD_STACK_CONTEXT)GetProp(hwndDlg, L"Context");
            PhDereferenceObject(threadStackContext->SymbolProvider);

            RemoveProp(hwndDlg, L"Context");
            RemoveProp(hwndDlg, L"LayoutManager");
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case IDCANCEL: // Esc and X button to close
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_REFRESH:
                {
                    PhpRefreshThreadStack(
                        (PTHREAD_STACK_CONTEXT)GetProp(hwndDlg, L"Context")
                        );
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PPH_LAYOUT_MANAGER layoutManager;

            layoutManager = (PPH_LAYOUT_MANAGER)GetProp(hwndDlg, L"LayoutManager");
            PhLayoutManagerLayout(layoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 250, 350);
        }
        break;
    }

    return FALSE;
}

static BOOLEAN NTAPI PhpWalkThreadStackCallback(
    __in PPH_THREAD_STACK_FRAME StackFrame,
    __in PVOID Context
    )
{
    PTHREAD_STACK_CONTEXT threadStackContext = (PTHREAD_STACK_CONTEXT)Context;
    PPH_STRING symbol;

    symbol = PhGetSymbolFromAddress(
        threadStackContext->SymbolProvider,
        (ULONG64)StackFrame->PcAddress,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (symbol)
    {
        PhAddListViewItem(threadStackContext->ListViewHandle, MAXINT,
            symbol->Buffer, NULL);
        PhDereferenceObject(symbol);
    }
    else
    {
        PhAddListViewItem(threadStackContext->ListViewHandle, MAXINT,
            L"???", NULL);
    }

    return TRUE;
}

static NTSTATUS PhpRefreshThreadStack(
    __in PTHREAD_STACK_CONTEXT ThreadStackContext
    )
{
    ListView_DeleteAllItems(ThreadStackContext->ListViewHandle);

    return PhWalkThreadStack(
        ThreadStackContext->ThreadHandle,
        ThreadStackContext->SymbolProvider->ProcessHandle,
        PH_WALK_I386_STACK | PH_WALK_AMD64_STACK | PH_WALK_KERNEL_STACK,
        PhpWalkThreadStackCallback,
        ThreadStackContext
        );
}

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
    THREAD_STACK_CONTEXT threadStackContext;

    threadStackContext.ProcessId = ProcessId;
    threadStackContext.ThreadId = ThreadId;
    threadStackContext.SymbolProvider = SymbolProvider;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDSTACK),
        ParentWindowHandle,
        PhpThreadStackDlgProc,
        (LPARAM)&threadStackContext
        );
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
            HWND lvHandle;

            threadStackContext = (PTHREAD_STACK_CONTEXT)lParam;
            SetProp(hwndDlg, L"Context", (HANDLE)threadStackContext);

            lvHandle = GetDlgItem(hwndDlg, IDC_THRDSTACK_LIST);
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"Name");

            {
                NTSTATUS status;
                HANDLE threadHandle = NULL;

                if (!NT_SUCCESS(status = PhOpenThread(
                    &threadHandle,
                    THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
                    threadStackContext->ThreadId
                    )))
                {
                    if (PhKphHandle)
                    {
                        status = PhOpenThread(
                            &threadHandle,
                            ThreadQueryAccess,
                            threadStackContext->ThreadId
                            );
                    }
                }

                if (!NT_SUCCESS(status))
                {
                    PhShowStatus(hwndDlg, L"Unable to open the thread", status, 0);
                    EndDialog(hwndDlg, 0);
                }

                threadStackContext->ThreadHandle = threadHandle;
            }

            threadStackContext->ListViewHandle = lvHandle;

            PhpRefreshThreadStack(threadStackContext);
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"Context");
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case IDCLOSE:
                EndDialog(hwndDlg, IDCLOSE);
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
